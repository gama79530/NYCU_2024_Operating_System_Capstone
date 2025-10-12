#include "common.hpp"
#include "kernel.hpp"
#include <new>

alignas(Kernel) static char kernelBuffer[sizeof(Kernel)];
Kernel *kernel = nullptr;

int32_t main(uint64_t x0)
{
    // initialize kernel instance
    kernel = new (kernelBuffer) Kernel(x0);
    kernel->run();
    
    return 0;
}
