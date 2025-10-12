#ifndef LAB2_CPP_MINI_UART_HPP
#define LAB2_CPP_MINI_UART_HPP
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