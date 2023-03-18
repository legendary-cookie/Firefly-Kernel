#pragma once

#include <cstddef>

#include "firefly/fs/stat.hpp"
#include "firefly/panic.hpp"

namespace firefly::kernel::fs {

class Resource {
public:
    using off_t = size_t;
    // ssize: -1 == error
    // is represented by SIZE_MAX
    using ssize_t = size_t;

    Stat st;

    // default implementations are provided as not every resource supports every type of I/O operation
    // will need proper error handling later
    virtual ssize_t read(void* buf, off_t offset, size_t count) {
        panic("Read not implemented by resource");
    }

    virtual ssize_t write(const void* buf, off_t offset, size_t count) {
        panic("Write not implemented by resource");
    }
};
}  // namespace firefly::kernel::fs
