#ifndef __KERNEL_HPP__
#define __KERNEL_HPP__
#include "mailbox.hpp"
#include "mini_uart.hpp"
#include "printf.hpp"
#include "shell.hpp"
#include "types.hpp"

class Kernel final
{
public:
    MiniUart miniUart;
    Printer printer;
    Shell shell;
    Mailbox mailbox;

    Kernel();
    // Shell
    void run();
};

extern Kernel *kernel;

#endif