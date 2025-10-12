#ifndef LAB2_CPP_KERNEL_HPP
#define LAB2_CPP_KERNEL_HPP
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