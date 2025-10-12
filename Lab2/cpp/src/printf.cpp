#include "printf.hpp"
#include "common.hpp"

const char *Printer::lowerDigits = "0123456789abcdef";
const char *Printer::upperDigits = "0123456789ABCDEF";

Printer::Printer(MiniUart *miniUart) : miniUart(miniUart) {}

void Printer::printf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}


void Printer::printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void Printer::sprintf(char *s, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void Printer::sprintf(char *s, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void Printer::vprintf(char *fmt, va_list args)
{
    format((void *) this->miniUart, Printer::printfPutc, fmt, args);
}

void Printer::vprintf(const char *fmt, va_list args)
{
    this->vprintf(const_cast<char *>(fmt), args);
}

void Printer::vsprintf(char *s, char *fmt, va_list args)
{
    format((void *) &s, Printer::sprintfPutc, fmt, args);
}

void Printer::vsprintf(char *s, const char *fmt, va_list args)
{
    this->vsprintf(s, const_cast<char *>(fmt), args);
}

void Printer::printfPutc(void *p, char c)
{
    MiniUart *miniUart = static_cast<MiniUart *>(p);
    if (c == '\n') {
        miniUart->putc('\r');
    }
    miniUart->putc(c);
}

void Printer::sprintfPutc(void *p, char c)
{
    *(*((char **) p))++ = c;
}

#if PRINTF_LONG_SUPPORT == true
void Printer::uli2a(uint64_t num, uint32_t base, int32_t uc, char *bf)
{
    const char *digits = uc ? upperDigits : lowerDigits;
#if HAS_DIVIDER == true
    int idx = 0;
    uint64_t divisor = 1;
    while (num / divisor >= base)
        divisor *= base;
    while (divisor != 0) {
        int digit = num / divisor;
        num %= divisor;
        divisor /= base;
        if (idx != 0 || digit > 0 || divisor == 0) {
            bf[idx++] = digits[digit];
        }
    }
    *bf = '\0';
#else
    int begin = 0;
    int end = 0;
    do {
        bf[end++] = digits[num % base];
        num /= base;
    } while (num != 0);
    bf[end--] = '\0';
    while (begin < end) {
        util::swap(bf[begin++], bf[end--]);
    }
#endif
}

void Printer::li2a(int64_t num, char *bf)
{
    if (num < 0) {
        num = -num;
        *bf++ = '-';
    }
    uli2a((uint64_t) num, 10, 0, bf);
}
#endif

void Printer::ui2a(uint32_t num, uint32_t base, int32_t uc, char *bf)
{
    const char *digits = uc ? upperDigits : lowerDigits;
#if HAS_DIVIDER == true
    int idx = 0;
    uint32_t divisor = 1;
    while (num / divisor >= base)
        divisor *= base;
    while (divisor != 0) {
        int digit = num / divisor;
        num %= divisor;
        divisor /= base;
        if (idx != 0 || digit > 0 || divisor == 0) {
            bf[idx++] = digits[digit];
        }
    }
    *bf = '\0';
#else
    int begin = 0;
    int end = 0;
    do {
        bf[end++] = digits[num % base];
        num /= base;
    } while (num != 0);
    bf[end--] = '\0';
    while (begin < end) {
        util::swap(bf[begin++], bf[end--]);
    }
#endif
}

void Printer::i2a(int32_t num, char *bf)
{
    if (num < 0) {
        num = -num;
        *bf++ = '-';
    }
    ui2a((uint32_t) num, 10, 0, bf);
}

int32_t Printer::a2d(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    else
        return -1;
}

char Printer::a2i(char ch, char **src, int32_t base, int32_t *nump)
{
    char *p = *src;
    int num = 0;
    int digit;
    while ((digit = a2d(ch)) >= 0) {
        if (digit > base)
            break;
        num = num * base + digit;
        ch = *p++;
    }
    *src = p;
    *nump = num;
    return ch;
}

void Printer::putWithWidth(void *putp, putcf putf, int32_t width, bool padZero, char *str)
{
    char padC = padZero ? '0' : ' ';
    char c;
    char *p = str;
    while (*p++ && width > 0) {
        width--;
    }
    while (width-- > 0) {
        putf(putp, padC);
    }
    while ((c = *str++)) {
        putf(putp, c);
    }
}

void Printer::format(void *putp, putcf putf, char *fmt, va_list args)
{
    char buffer[32];
    char c;

    while ((c = *fmt++)) {
        if (c != '%') {
            putf(putp, c);
        } else {
            bool padZero = false;
            int32_t width = 0;
#if PRINTF_LONG_SUPPORT == true
            bool isLong = false;
#endif
            c = *fmt++;
            if (c == '0') {
                padZero = true;
                c = *fmt++;
            }
            if (c >= '0' && c <= '9') {
                c = a2i(c, &fmt, 10, &width);
            }
#if PRINTF_LONG_SUPPORT == true
            if (c == 'l') {
                isLong = true;
                c = *fmt++;
            }
#endif
            switch (c) {
            case 'u':
#if PRINTF_LONG_SUPPORT == true
                if (isLong)
                    uli2a(va_arg(args, uint64_t), 10, 0, buffer);
                else
#endif
                    ui2a(va_arg(args, uint32_t), 10, 0, buffer);
                putWithWidth(putp, putf, width, padZero, buffer);
                break;
            case 'd':
#if PRINTF_LONG_SUPPORT == true
                if (isLong)
                    li2a(va_arg(args, int64_t), buffer);
                else
#endif
                    i2a(va_arg(args, int32_t), buffer);
                putWithWidth(putp, putf, width, padZero, buffer);
                break;
            case 'x':
            case 'X':
#if PRINTF_LONG_SUPPORT == true
                if (isLong)
                    uli2a(va_arg(args, uint64_t), 16, (c == 'X'), buffer);
                else
#endif
                    ui2a(va_arg(args, uint32_t), 16, (c == 'X'), buffer);
                putWithWidth(putp, putf, width, padZero, buffer);
                break;
            case 'c':
                putf(putp, (char) (va_arg(args, int)));
                break;
            case 's':
                putWithWidth(putp, putf, width, false, va_arg(args, char *));
                break;
            case '%':
                putf(putp, c);
            default:
                break;
            }
        }
    }
}