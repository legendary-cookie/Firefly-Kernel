#pragma once

#include "firefly/fs/resource.hpp"
#include "firefly/fs/vfs.hpp"
#include "frg/spinlock.hpp"

namespace firefly::kernel::fs {

class TmpfsResource : public Resource {
    friend class Tmpfs;

public:
    ssize_t read(void* buf, off_t offset, size_t count);
    ssize_t write(const void* buf, off_t offset, size_t count);
    TmpfsResource();

protected:
    void* data;
    size_t capacity;

private:
    frg::ticket_spinlock lock = frg::ticket_spinlock();
};

class Tmpfs : public Filesystem {
    friend class TmpfsResource;

public:
    Node* create(Node* parent, frg::string_view name, int mode);

    Node* symlink(Node* parent, frg::string_view name, frg::string_view target);

    void populate(Node* directory) {
    }

    static void init();

protected:
    uint64_t inodeCount;
};
}  // namespace firefly::kernel::fs
