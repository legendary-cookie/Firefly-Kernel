#pragma once

#include "firefly/fs/resource.hpp"
#include "firefly/memory-manager/allocator.hpp"
#include "firefly/memory-manager/secondary/heap.hpp"
#include "frg/hash.hpp"
#include "frg/hash_map.hpp"
#include "frg/manual_box.hpp"
#include "frg/spinlock.hpp"
#include "frg/string.hpp"
#include "frg/tuple.hpp"
#include "frg/vector.hpp"

namespace firefly::kernel::fs {

class Filesystem;

struct Node {
    // TODO: maybe fix the +2 by rounding up in the allocator (min size) "" is passed on root creation
    Node(Filesystem* fs, Node* parent, frg::string_view _name, bool directory)
        : filesystem{ fs }, parent{ parent }, children{ frg::hash<frg::string_view>{} }, name{ _name }, directory{ directory } {
    }

    Filesystem* filesystem;
    Node* parent;
    Node* mountpoint;
    Node* redir;
    Resource* resource;
    frg::hash_map<frg::string_view, Node*, frg::hash<frg::string_view>, Allocator> children;
    frg::string<Allocator> name;
    frg::string<Allocator> symlink_target;
    bool directory;
    bool populated = false;
};

class Filesystem {
public:
    // create a new file/dir on the filesystem
    virtual Node* create(Node* parent, frg::string_view name, int mode) = 0;

    // populate children by e.g. reading directory entries
    virtual void populate(Node* directory) {
        directory->populated = true;
    };

    virtual Node* symlink(Node* parent, frg::string_view name, frg::string_view target) = 0;
};


class VFS {
    using MountFunction = Node* (*)(Node*, frg::string_view, Node*);

public:
    VFS()
        : filesystems{ frg::hash<frg::string_view>{}, Allocator() } {
    }
    ~VFS() {
    }

    static void init();
    static VFS& accessor();

public:
    void registerFilesystem(MountFunction fs, frg::string_view fsname);

    bool mount(Node* parent, frg::string_view source, frg::string_view target, frg::string_view fs_name);

    // Tuple:
    // - target parent
    // - target
    frg::tuple<Node*, Node*> parsePath(Node* parent, frg::string_view path);

    Node* create(Node* parent, frg::string_view name, int mode);

private:
    Node* reduce(Node* node, bool followSymlinks);

public:
    Node* root = nullptr;

private:
    frg::hash_map<
        frg::string_view,
        MountFunction,
        frg::hash<frg::string_view>,
        Allocator>
        filesystems;

    frg::ticket_spinlock lock = frg::ticket_spinlock();
};


}  // namespace firefly::kernel::fs
