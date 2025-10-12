#ifndef LAB2_CPP_KERNEL_HPP
#define LAB2_CPP_KERNEL_HPP
#include "mailbox.hpp"
#include "mini_uart.hpp"
#include "printf.hpp"
#include "shell.hpp"
#include "types.hpp"
#include "device_tree.hpp"
#include "cstring.hpp"

class Kernel final
{
public:
    MiniUart miniUart;
    Printer printer;
    Shell shell;
    Mailbox mailbox;
    DeviceTree deviceTree;

    Kernel(uint64_t x0);
    // Shell
    void run();
};

extern Kernel *kernel;

#endif