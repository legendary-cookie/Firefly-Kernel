#include "firefly/fs/vfs.hpp"

#include <cstring>

#include "firefly/logger.hpp"
#include "firefly/memory-manager/primary/primary_phys.hpp"
#include "firefly/memory-manager/secondary/heap.hpp"
#include "firefly/panic.hpp"
#include "frg/manual_box.hpp"
#include "frg/tuple.hpp"
#include "libk++/cstring.hpp"
#include "libk++/memory.hpp"

namespace firefly::kernel::fs {
namespace {
frg::manual_box<VFS> vfsSingleton;
}  // namespace


VFS& VFS::accessor() {
    return *vfsSingleton;
}

void VFS::init() {
    if (vfsSingleton.valid()) panic("Tried to init VFS twice");
    vfsSingleton.initialize();

    auto vfs = vfsSingleton.get();
    vfs->root = new Node(nullptr, nullptr, "", true);
}

bool VFS::mount(Node* parent, frg::string_view source, frg::string_view target, frg::string_view fs_name) {
    lock.lock();

    MountFunction fs_mount = *filesystems.get(fs_name);
    if (!fs_mount) {
        debugLine << "ENODEV\n"
                  << fmt::endl;
        return false;
    }

    Node* sourceNode = nullptr;

    if (source != "" && source.size() != 0) {
        auto res = parsePath(parent, source);
        sourceNode = res.get<1>();
        if (sourceNode == nullptr) {
            return false;
        }
        if (S_ISDIR(sourceNode->resource->st.st_mode)) {
            debugLine << "Tried to mount root to directory\n"
                      << fmt::endl;
            return false;
        }
    }

    auto res = parsePath(parent, target);
    auto targetParent = res.get<0>();
    auto targetNode = res.get<1>();

    bool mounting_root = targetNode == root;

    if (!mounting_root && !targetNode->directory) {
        debugLine << "Mount error: " << target.data() << "isn't a directory\n"
                  << fmt::endl;
        return false;
    }

    Node* mountNode = fs_mount(targetParent, fs_name, sourceNode);

    if (!mountNode) {
        debugLine << "Mount error: Mounting failed\n"
                  << fmt::endl;
        return false;
    }

    targetNode->mountpoint = mountNode;

    debugLine << "Mounted " << source.data() << " to " << target.data() << '\n'
              << fmt::endl;

    lock.unlock();
    return true;
}

void VFS::registerFilesystem(MountFunction fn, frg::string_view fsname) {
    filesystems.insert(fsname, fn);
}

Node* VFS::reduce(Node* node, bool followSymlinks) {
    if (node->redir != nullptr) {
        return reduce(node->redir, followSymlinks);
    }
    if (node->mountpoint != nullptr) {
        return reduce(node->mountpoint, followSymlinks);
    }

    if (followSymlinks && node->symlink_target != "") {
        auto res = parsePath(node->parent, node->symlink_target);
        if (!res.get<1>())
            return nullptr;
        return reduce(res.get<1>(), followSymlinks);
    }

    return node;
}

// Taken from Lyre Kernel:
// I want to change this, maybe make it more 'c++'y
// probably temporary solution
frg::tuple<Node*, Node*> VFS::parsePath(Node* parent, frg::string_view path) {
    if (path.size() == 0) {
        debugLine << "ENOENT\n"
                  << fmt::endl;
        return frg::make_tuple(nullptr, nullptr);
    }

    bool ask_for_dir = path[path.size() - 1] == '/';

    size_t index = 0;
    Node* current_node = reduce(parent, false);

    // populate

    if (path[index] == '/') {
        current_node = reduce(this->root, false);
        while (path[index] == '/') {
            if (index == path.size() - 1) {
                return frg::make_tuple(current_node, current_node);
            }
            index++;
        }
    }

    for (;;) {
        const char* elem = &path[index];
        size_t elem_len = 0;

        while (index < path.size() && path[index] != '/') {
            elem_len++;
            index++;
        }

        while (index < path.size() && path[index] == '/') {
            index++;
        }

        bool last = index == path.size();

        // TODO: Figure out WHY allocating with heap alloc panics
        char* elem_str = static_cast<char*>(mm::Physical::allocate(elem_len + 1));
        memcpy(elem_str, elem, elem_len);

        current_node = reduce(current_node, false);

        Node** nodes = current_node->children.get(elem_str);
        if (nodes == nullptr) {
            debugLine << "ENOENT\n"
                      << fmt::endl;
            if (last) {
                return frg::make_tuple(current_node, nullptr);
            }
            return frg::make_tuple(nullptr, nullptr);
        }

        auto new_node = *nodes;

        new_node = reduce(new_node, false);

        // populate

        if (last) {
            if (ask_for_dir && !S_ISDIR(new_node->resource->st.st_mode)) {
                debugLine << "ENOTDIR\n"
                          << fmt::endl;
                return frg::make_tuple(current_node, nullptr);
            }
            return frg::make_tuple(current_node, new_node);
        }

        current_node = new_node;

        // check for link

        if (!S_ISDIR(current_node->resource->st.st_mode)) {
            debugLine << "ENOTDIR\n"
                      << fmt::endl;
            return frg::make_tuple(nullptr, nullptr);
        }
    }

    debugLine << "ENOENT\n"
              << fmt::endl;
    return frg::make_tuple(nullptr, nullptr);
}

Node* VFS::create(Node* parent, frg::string_view name, int mode) {
    lock.lock();

    auto res = parsePath(parent, name);
    // debugLine << "CREATE RES #0 0x" << fmt::hex << reinterpret_cast<uint64_t>(res.get<0>()) << " RES #1 0x" << fmt::hex << reinterpret_cast<uintptr_t>(res.get<1>()) << '\n' << fmt::endl;

    if (!res.get<0>()) {
        panic("Couldn't find path");
        return nullptr;
    }
    if (res.get<1>()) {
        panic("Path already exists");
        return nullptr;
    }

    auto fs = res.get<0>()->filesystem;
    auto lastSlash = name.find_last('/');
    auto basename = name.sub_string(lastSlash + 1, name.size() - lastSlash - 1);
    auto node = fs->create(res.get<0>(), basename, mode);

    // insert into parent directory entries
    res.get<0>()->children.insert(basename, node);

    lock.unlock();
    return node;
}

}  // namespace firefly::kernel::fs
