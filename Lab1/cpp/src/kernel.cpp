#include "kernel.hpp"

Kernel::Kernel() : miniUart(), printer(&miniUart), shell(), mailbox() {}

void Kernel::run()
{
    shell.run();
}
