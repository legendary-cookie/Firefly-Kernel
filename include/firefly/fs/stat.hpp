#pragma once

#include <cstddef>
#include <cstdint>

namespace firefly::kernel {

constexpr int S_IFMT = 0170000;
constexpr int S_IFDIR = 0040000;
constexpr int S_IFREG = 0100000;
constexpr int S_IFCHR = 0020000;
constexpr int S_IFBLK = 0060000;
constexpr int S_IFIFO = 0010000;
constexpr int S_IFLNK = 0120000;
constexpr int S_IFSOCK = 0140000;

constexpr int S_ISUID = 04000;
constexpr int S_ISGID = 02000;
constexpr int S_ISVT = 01000;
constexpr int S_IREAD = 0400;
constexpr int S_IWRITE = 0200;
constexpr int S_IEXEC = 0100;

constexpr bool S_ISREG(int mode) {
    return (mode & S_IFMT) == S_IFREG;
}

constexpr bool S_ISDIR(int mode) {
    return (mode & S_IFMT) == S_IFDIR;
}

struct Stat {
    size_t st_size = 0;
    uint64_t st_ino;
    int st_mode;
};

}  // namespace firefly::kernel
