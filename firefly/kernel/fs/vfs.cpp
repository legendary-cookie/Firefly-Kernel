#include "firefly/fs/vfs.hpp"

#include "firefly/logger.hpp"
#include "firefly/memory-manager/secondary/heap.hpp"
#include "firefly/panic.hpp"
#include "frg/manual_box.hpp"
#include "frg/tuple.hpp"
#include "libk++/cstring.hpp"

namespace firefly::kernel::fs {
namespace {
frg::manual_box<VFS> vfsSingleton;
}  // namespace

Node* VFS::createNode(Filesystem* fs, Node* parent, frg::string_view name, bool directory) {
    Node* node = new Node(fs, parent, name, directory);
    return node;
}

VFS& VFS::accessor() {
    return *vfsSingleton;
}

void VFS::init() {
    if (vfsSingleton.valid()) panic("Tried to init twice");
    vfsSingleton.initialize();

    auto vfs = vfsSingleton.get();
    vfs->root = vfs->createNode(nullptr, nullptr, "", true);
}

bool VFS::mount(Node* parent, frg::string_view source, frg::string_view target, frg::string_view fs_name) {
    MountFunction fs_mount = *filesystems.get(fs_name);
    if (!fs_mount)
        return false;

    Node* sourceNode = nullptr;
    if (source != "" && source.size() != 0) {
        auto res = parsePath(parent, source);
        sourceNode = res.get<1>();
        if (sourceNode == nullptr) panic("VFS null");
        if (sourceNode->directory) panic("directory");
    }

    auto res = parsePath(parent, target);
    bool mounting_root = res.get<1>() == root;
    if (!mounting_root && !res.get<1>()->directory) panic("no directory");
    if (res.get<1>() == nullptr) panic("== null");

    Node* mountNode = fs_mount(res.get<0>(), fs_name, sourceNode);
    if (!mountNode)
        panic("Couldnt mount");

    res.get<1>()->mountpoint = mountNode;

    return true;
}

void VFS::registerFilesystem(MountFunction fn, frg::string_view fsname) {
    filesystems.insert(fsname, fn);
}

Node* VFS::reduce(Node* node) {
    if (node->mountpoint != nullptr) {
        return reduce(node->mountpoint);
    }

    return node;
}

// Taken from Lyre Kernel:
// I want to change this, maybe make it more 'c++'y
// temporary solution
frg::tuple<Node*, Node*> VFS::parsePath(Node* parent, frg::string_view path) {
    if (path.size() == 0) {
        return frg::make_tuple(nullptr, nullptr);
    }

    auto pathLen = path.size();

    bool ask_for_dir = path[pathLen - 1] == '/';

    size_t index = 0;

    Node* currentNode = reduce(parent);

    // populate

    if (path[index] == '/') {
        currentNode = reduce(this->root);

        while (path[index] == '/') {
            if (index == pathLen - 1) {
                return frg::make_tuple(currentNode, currentNode);
            }
            index++;
        }
    }

    for (;;) {
        const char* elem = &path[index];
        size_t elemLen = 0;

        while (index < pathLen && path[index] != '/') {
            elemLen++, index++;
        }

        while (index < pathLen && path[index] == '/') {
            index++;
        }

        bool last = index == pathLen;

        char* elemStr = (char*)mm::heap->allocate(elemLen + 1);
        memcpy(elemStr, elem, elemLen);

        currentNode = reduce(currentNode);

        Node* newNode = *currentNode->children.get(elemStr);
        if (!newNode) {
            if (last) {
                return frg::make_tuple(currentNode, nullptr);
            }
            return frg::make_tuple(nullptr, nullptr);
        }

        newNode = reduce(newNode);
        // populate

        if (last) {
            if (ask_for_dir && !newNode->directory) {
                return frg::make_tuple(currentNode, nullptr);
            }
            return frg::make_tuple(currentNode, newNode);
        }

        currentNode = newNode;

        if (!currentNode->directory) {
            return frg::make_tuple(nullptr, nullptr);
        }
    }

    return frg::make_tuple(nullptr, nullptr);
}

}  // namespace firefly::kernel::fs
