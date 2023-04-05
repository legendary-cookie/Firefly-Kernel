#include "firefly/memory-manager/secondary/heap.hpp"

#include <frg/list.hpp>

#include "cstdlib/cmath.h"
#include "firefly/logger.hpp"
#include "firefly/memory-manager/primary/buddy.hpp"
#include "firefly/memory-manager/secondary/slab/slab.hpp"
#include "firefly/memory-manager/virtual/virtual.hpp"

namespace firefly::kernel::mm {

constexpr bool debugHeap = false;

BuddyAllocator vm_buddy;

// Todo:
// - The slab allocator needs to resize itself (i.e. add or remove(free) slabs).
// (Useful when the system is low on memory)
//
// - Integrate kasan into this.
struct VmBackingAllocator {
    VirtualAddress allocate(int size) {
        auto ptr = vm_buddy.alloc(size).unpack();

        if (ptr == nullptr)
            panic("vm_buddy returned a null-pointer, heap is OOM.");

        if constexpr (debugHeap) {
            debugLine << "Heap VM Allocated " << fmt::hex << reinterpret_cast<uintptr_t>(ptr) << '\n'
                      << fmt::endl;
        }

        return VirtualAddress(ptr);
    }
};

struct LockingMechanism {
    void lock() {
    }
    void unlock() {
    }
};


// Allocatable sizes: 8, 16, 32 64, 128, 256, 512, 1024, 2048, 4096.
// Allocation requests for sizes lower than the minimum, 8, will be served with the minimum size (8).
using cacheType = slabCache<VmBackingAllocator, LockingMechanism>;
frg::array<cacheType, 10> kernelAllocatorCaches = {};

constinit frg::manual_box<kernelHeap> heap;

void kernelHeap::init() {
    // Map kernel heap with physical memory to back it up.
    for (int i = 0; i < GiB(1); i += PageSize::Size4K) {
        auto phys = reinterpret_cast<uintptr_t>(Physical::must_allocate());
        kernelPageSpace::accessor().map(i + AddressLayout::SlabHeap, phys, AccessFlags::ReadWrite, PageSize::Size4K);
    }

    // Initialize separate buddy instance to manage heap memory (VmBackingAllocator)
    vm_buddy.init(reinterpret_cast<uint64_t*>(AddressLayout::SlabHeap), BuddyAllocator::largest_allowed_order);

    heap.initialize();
    for (size_t i = 0, j = 3; i < kernelAllocatorCaches.size(); i++, j++) {
        if constexpr (debugHeap) {
            debugLine << "init cache " << fmt::dec << i << ", with size " << fmt::dec << (1 << j) << '\n'
                      << fmt::endl;
        }

        kernelAllocatorCaches[i].initialize(1 << j, "heap");
    }
}

VirtualAddress kernelHeap::allocate(size_t size) const {
    if (size < 8) {
        if constexpr (debugHeap) {
            debugLine << "heap alloc size under smallest\n"
                      << fmt::endl;
        }
        size = 8;
    }

    assert_truth(size <= 4096 && "Invalid allocation size");

    if (!slabHelper::powerOfTwo(size))
        size = slabHelper::alignToSecondPower(size);

    auto& cache = kernelAllocatorCaches[log2(size) - 2 - 1];
    auto ptr = cache.allocate();

    if constexpr (debugHeap) {
        debugLine << "Heap allocated (size=" << fmt::dec << size << "): 0x" << fmt::hex << reinterpret_cast<uintptr_t>(ptr) << '\n'
                  << fmt::endl;
    }

    return ptr;
}

void kernelHeap::deallocate(VirtualAddress ptr) const {
    if (ptr == nullptr)
        return;

    auto aligned_address = (reinterpret_cast<uintptr_t>(ptr) >> PAGE_SHIFT) << PAGE_SHIFT;

    auto size = pagelist.phys_to_page(aligned_address - AddressLayout::SlabHeap)->slab_size;

    assert_truth(size <= 4096 && "Invalid deallocation size");
    if (size < 8) return;

    auto& cache = kernelAllocatorCaches[log2(size) - 2 - 1];
    cache.deallocate(ptr);
}

}  // namespace firefly::kernel::mm
