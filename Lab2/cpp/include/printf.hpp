#ifndef LAB2_CPP_PRINTF_HPP
#define LAB2_CPP_PRINTF_HPP
#include <cstdarg>
#include "mini_uart.hpp"

#define PRINTF_LONG_SUPPORT true
#define HAS_DIVIDER false

class Printer
{
    using putcf = void (*)(void *, char);

public:
    Printer(MiniUart *miniUart);
    void printf(char *fmt, ...);
    void printf(const char *fmt, ...);
    void sprintf(char *s, char *fmt, ...);
    void sprintf(char *s, const char *fmt, ...);
    void vprintf(char *fmt, va_list args);
    void vprintf(const char *fmt, va_list args);
    void vsprintf(char *s, char *fmt, va_list args);
    void vsprintf(char *s, const char *fmt, va_list args);

private:
    static const char *lowerDigits;
    static const char *upperDigits;

    static void printfPutc(void *p, char c);
    static void sprintfPutc(void *p, char c);

    MiniUart *miniUart = nullptr;

#if PRINTF_LONG_SUPPORT == true
    void uli2a(uint64_t num, uint32_t base, int32_t uc, char *bf);
    void li2a(int64_t num, char *bf);
#endif
    void ui2a(uint32_t num, uint32_t base, int32_t uc, char *bf);
    void i2a(int32_t num, char *bf);
    int32_t a2d(char ch);
    char a2i(char ch, char **src, int32_t base, int32_t *nump);
    void putWithWidth(void *putp, putcf putf, int32_t width, bool padZero, char *str);
    void format(void *putp, putcf putf, char *fmt, va_list args);
};


#endif