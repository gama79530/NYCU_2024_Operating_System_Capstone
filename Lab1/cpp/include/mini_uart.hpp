#ifndef __MINI_UART_HPP__
#define __MINI_UART_HPP__
#include "types.hpp"

class MiniUart
{
public:
    MiniUart();

    virtual char getb();
    virtual char getc();
    virtual void putc(char c);
    virtual void puts(const char *s);
    virtual void putln(const char *s);
};

#endif