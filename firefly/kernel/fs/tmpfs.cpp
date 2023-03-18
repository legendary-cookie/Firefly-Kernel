#include "firefly/fs/tmpfs.hpp"

#include "firefly/fs/resource.hpp"
#include "firefly/fs/vfs.hpp"
#include "firefly/logger.hpp"
#include "firefly/memory-manager/primary/primary_phys.hpp"
#include "firefly/memory-manager/secondary/heap.hpp"
#include "libk++/memory.hpp"

namespace firefly::kernel::fs {

TmpfsResource::TmpfsResource() {
}

Node* Tmpfs::create(Node* parent, frg::string_view name, bool directory, int mode) {
    Node* node = new Node(this, parent, name, S_ISDIR(mode));
    TmpfsResource* res = new TmpfsResource();

    if (!res)
        panic("couldn't create tmpfs resource");

    node->resource = res;

    if (S_ISREG(mode)) {
        res->capacity = 4096;
        res->data = mm::Physical::must_allocate(res->capacity);
    }

    res->st.st_size = 0;
    res->st.st_ino = inodeCount++;
    res->st.st_mode = mode;

    return node;
}

Resource::ssize_t TmpfsResource::write(const void* buf, Resource::off_t offset, size_t count) {
    lock.lock();
    ssize_t ret = -1;

    if (offset + count >= this->capacity) {
        size_t new_capacity = this->capacity;
        while (offset + count >= new_capacity) {
            new_capacity *= 2;
        }
        // TODO: implement realloc
        void* new_data = mm::Physical::must_allocate(new_capacity);
        memcpy(new_data, this->data, this->capacity);
        mm::Physical::deallocate(this->data);
        this->data = new_data;
        this->capacity = new_capacity;
    }

    memcpy((void*)((off_t)this->data + offset), buf, count);

    if ((offset + count) >= this->st.st_size) {
        this->st.st_size = offset + count;
    }

    ret = count;

    lock.unlock();
    return ret;
}

Resource::ssize_t TmpfsResource::read(void* buf, Resource::off_t offset, size_t count) {
    lock.lock();

    size_t actual_count = count;

    if (offset + count >= this->st.st_size) {
        actual_count = count - ((offset + count) - this->st.st_size);
    }

    memcpy(buf, (void*)((off_t)this->data + offset), count);

    lock.unlock();
    return actual_count;
}

static inline Tmpfs* tmpfs_instance() {
    return new Tmpfs();
}

Node* tmpfs_mount(Node* parent, frg::string_view name, Node* source) {
    Tmpfs* new_fs = tmpfs_instance();
    Node* ret = new_fs->create(
        parent, name, true, 0644 | S_IFDIR);
    return ret;
}

void Tmpfs::init() {
    VFS::accessor().registerFilesystem(tmpfs_mount, "tmpfs");
}

}  // namespace firefly::kernel::fs
