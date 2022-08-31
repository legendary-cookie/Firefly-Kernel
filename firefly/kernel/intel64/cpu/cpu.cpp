#include "firefly/intel64/cpu/cpu.hpp"

// Note: This is purposely bare-bones and lacking a
// proper "per cpu" structure (i.e. gsbase).
// There is no multi-core yet, but I'd prefer to structure
// everything in a way so that it's easy to get it up and running now rather than later.
namespace firefly::kernel {
CpuData temporaryCpuInstance;

void initializeThisCpu(uint64_t stack) {
    core::gdt::init(temporaryCpuInstance.gdt);
	core::tss::init(stack);
}

CpuData &thisCpu() {
    return temporaryCpuInstance;
}
}  // namespace firefly::kernel