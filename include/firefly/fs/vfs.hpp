#pragma once

#include "firefly/fs/resource.hpp"
#include "firefly/memory-manager/allocator.hpp"
#include "firefly/memory-manager/secondary/heap.hpp"
#include "frg/hash.hpp"
#include "frg/hash_map.hpp"
#include "frg/manual_box.hpp"
#include "frg/string.hpp"
#include "frg/tuple.hpp"
#include "frg/vector.hpp"
#include "libk++/cstring.hpp"

namespace firefly::kernel::fs {

struct Node;

using MountFunction = Node* (*)(Node*, frg::string_view, Node*);

class Filesystem {
public:
    // create a new file/dir on the filesystem
    // TODO: maybe stat modes for directories?
    virtual Node* create(Node* parent, frg::string_view name, bool directory) = 0;

    // populate children by e.g. reading directory entries
    virtual void populate(Node* directory) = 0;
};

struct Node {
    // TODO: maybe fix the +2 by rounding up in the allocator (min size) "" is passed on root creation
    Node(Filesystem* fs, Node* parent, frg::string_view _name, bool directory)
        : filesystem{ fs }, parent{ parent }, children{ {}, Allocator() }, directory{ directory } {
    }

    Filesystem* filesystem;
    Node* parent;
    Node* mountpoint;
    Resource* resource;
    frg::hash_map<frg::string_view, Node*, frg::hash<frg::string_view>, Allocator> children;
    frg::string<Allocator> name;
    bool directory;
};

class VFS {
public:
    VFS()
        : filesystems{ frg::hash<frg::string_view>{}, Allocator() } {
    }
    ~VFS() {
    }

    static void init();
    static VFS& accessor();

public:
    Node* createNode(Filesystem* fs, Node* parent, frg::string_view name, bool directory);
    void registerFilesystem(MountFunction fs, frg::string_view fsname);

    bool mount(Node* parent, frg::string_view source, frg::string_view target, frg::string_view fs_name);

    // Tuple:
    // - target parent
    // - target
    frg::tuple<Node*, Node*> parsePath(Node* parent, frg::string_view path);

private:
    Node* reduce(Node* node);

public:
    Node* root = nullptr;

private:
    frg::hash_map<
        frg::string_view,
        MountFunction,
        frg::hash<frg::string_view>,
        Allocator>
        filesystems;
};


}  // namespace firefly::kernel::fs
