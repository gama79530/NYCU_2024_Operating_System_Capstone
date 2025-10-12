#include "kernel.hpp"

Kernel::Kernel(uint64_t x0) : miniUart(), printer(&miniUart), shell(), mailbox(), deviceTree()
{
    deviceTree.init(x0);
}

void Kernel::run()
{
    shell.run();
}
