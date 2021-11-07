#include <stl/array.h>
#include <stl/cstdlib/stdio.h>
#include <x86_64/libk++/iostream.h>

#include <x86_64/drivers/ps2.hpp>
#include <x86_64/drivers/serial.hpp>
#include <x86_64/drivers/vbe.hpp>
#include <x86_64/kernel.hpp>
#include <x86_64/trace/strace.hpp>
<<<<<<< HEAD
#include "x86_64/memory-manager/primary/primary_phys.hpp"
#include "x86_64/gdt/tss.hpp"
=======
>>>>>>> 956a1778f4af0c133c58be862eb4cee1354f9b13

[[maybe_unused]] constexpr short MAJOR_VERSION = 0;
[[maybe_unused]] constexpr short MINOR_VERSION = 0;
constexpr const char *VERSION_STRING = "0.0";

namespace firefly::kernel::main {
void write_ff_info() {
    printf("FireflyOS\nVersion: %s\nContributors:", VERSION_STRING);

    firefly::std::array<const char *, 3> arr = {
        "Lime\t  ", "JohnkaS", "V01D-NULL"
    };

    for (size_t i = 0; i < arr.max_size(); i++) {
        if (i % 2 == 0) {
            puts("\n\t");
        }
        printf("%s  ", arr[i]);
    }
    puts("\n");
}


[[noreturn]] void kernel_main() {
    // Never free rsp0
    auto rsp0 = firefly::kernel::mm::primary::allocate(1);
    if (rsp0 == nullptr) firefly::trace::panic("Failed to allocate memory for the TSS for core 0 (main core)");
    firefly::kernel::core::tss::core0_tss_init(reinterpret_cast<size_t>(rsp0->data[0]));
    
    trace::panic("Reached the end of kernel");
    __builtin_unreachable();
}
}  // namespace firefly::kernel::main