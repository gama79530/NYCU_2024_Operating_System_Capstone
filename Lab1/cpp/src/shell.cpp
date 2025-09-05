#include "shell.hpp"
#include "common.hpp"
#include "kernel.hpp"
#include "peripheral.hpp"
#include "power.hpp"

const char *const Shell::errorMessages[] = {"Reading error: command is too long.",
                                            "Parsing error: token length is too long.",
                                            "Empty command.", "Parsing error: too many tokens."};

Shell::Shell()
{
    buffer[SHELL_BUFFER_MAX_SIZE] = '\0';
    for (int i = 0; i < SHELL_TOKEN_MAX_NUM; i++) {
        tokens[i][SHELL_TOKEN_MAX_LEN] = '\0';
    }
}

void Shell::run()
{
    int32_t errCode = 0;
    kernel->printer.printf(
        "Welcome to the OGC shell. Use \"help\" for information on supported commands.\n");
    while (true) {
        newLine();
        if ((errCode = readCommand())) {
            kernel->printer.printf("\n%s\n", errorCodeToMessage(static_cast<ErrorCode>(errCode)));
            continue;
        }
        if ((errCode = parseCommand())) {
            kernel->printer.printf("\n%s\n", errorCodeToMessage(static_cast<ErrorCode>(errCode)));
            continue;
        }
        executeCommand();
    }
}

inline uint32_t Shell::bufferRemaining() const
{
    return SHELL_BUFFER_MAX_SIZE - bufferUsed;
};

inline const char *Shell::errorCodeToMessage(ErrorCode errorCode) const
{
    return errorMessages[~static_cast<uint32_t>(errorCode)];
}


void Shell::newLine()
{
    kernel->printer.printf("\r$ ");
    bufferUsed = 0;
}

int32_t Shell::readCommand()
{
    char c;
    do {
        c = kernel->miniUart.getc();
        if (c == '\n') {  // enter new line
            buffer[bufferUsed++] = '\0';
            kernel->printer.printf("\n");
            return 0;
        } else if (c == 0x7F) {  // c = del
            if (bufferUsed) {
                kernel->printer.printf(
                    "%c %c", (char) 0x8,
                    (char) 0x8);  // print 2 backspaces to remove del and one char
                bufferUsed--;
            }
        } else if (c > 0x1F) {
            buffer[bufferUsed++] = c;
            kernel->printer.printf("%c", c);
        }
    } while (c != '\n' && bufferRemaining());

    return ErrorCode::ERR_CODE_MSG_TOO_LONG;
}

int32_t Shell::parseCommand()
{
    const char *src = (const char *) buffer;
    char *dest;

    for (tokensCount = 0; *src != '\0' && tokensCount < SHELL_TOKEN_MAX_NUM; tokensCount++) {
        while (*src == ' ') {  // skip leading spaces before token
            src++;
        }

        dest = tokens[tokensCount];
        for (uint32_t tokenLen = 0; *src != ' ' && *src != '\0'; tokenLen++) {
            if (tokenLen >= SHELL_TOKEN_MAX_LEN) {
                return ErrorCode::ERR_CODE_TOO_LONG_TOKEN;
            }
            *(dest++) = *(src++);
        }
        *(dest) = '\0';
    }

    if (tokensCount == 0) {
        return ErrorCode::ERR_CODE_EMPTY_COMMAND;
    } else if (*src != '\0') {
        return ErrorCode::ERR_CODE_TOO_MANY_TOKENS;
    }

    return 0;
}

void Shell::executeCommand()
{
    if (!cstr::strcmp(tokens[0], "help")) {
        commandHelp();
    } else if (!cstr::strcmp(tokens[0], "hello")) {
        commandHello();
    } else if (!cstr::strcmp(tokens[0], "mailbox")) {
        commandMailbox();
    } else if (!cstr::strcmp(tokens[0], "reboot")) {
        commandReboot();
    } else {
        kernel->printer.printf(
            "Unsupported command: %s\n"
            "\n",
            buffer);
    }
}

inline bool Shell::isHelp() const
{
    for (uint32_t i = 1; i < tokensCount; i++) {
        if (!cstr::strcmp(tokens[i], "-h") || !cstr::strcmp(tokens[i], "-help")) {
            return true;
        }
    }
    return false;
}

void Shell::commandHelp()
{
    kernel->printer.printf(
        "The basic format of a command is \"{Command} [arg1 arg2 ...]\".\n"
        "Arguments are optional tokens: [arg], [-arg], or [-arg=xxx].\n"
        "Each token has a maximum length of %u characters, excluding the terminating null.\n"
        "Commands marked with '*' accept arguments.\n"
        "Use the argument \"-h\" or \"-help\" to find more details.\n"
        "\n"
        "help\t\t: Display the help menu.\n"
        "hello\t\t: Print \"Hello World!\"\n"
        "mailbox*\t: Communicate with the VideoCoreIV GPU.\n"
        "reboot\t\t: Reboot system.\n"
        "\n",
        SHELL_TOKEN_MAX_LEN);
}

void Shell::commandHello()
{
    kernel->printer.printf(
        "Hello world!\n"
        "\n");
}


void Shell::commandMailbox()
{
    if (isHelp()) {
        kernel->printer.printf(
            "<default>\t\t: with all optional arguments.\n"
            "[-revision]\t\t: Display the board revision.\n"
            "[-arm-memory-info]\t: Display the ARM memory base address and size.\n"
            "\n");
        return;
    }

    if (tokensCount == 1) {
        getBoardRevision();
        getArmMemoryInfo();
    } else {
        for (uint32_t i = 1; i < tokensCount; i++) {
            if (!cstr::strcmp(tokens[i], "-revision")) {
                getBoardRevision();
            } else if (!cstr::strcmp(tokens[i], "-arm-memory-info")) {
                getArmMemoryInfo();
            } else {
                kernel->printer.printf("Unsupported argument: %s\n", tokens[i]);
            }
        }
    }
    kernel->printer.printf("\n");
}

void Shell::getBoardRevision(void)
{
    kernel->mailbox.buffer[0] = 7 * 4;  // buffer size in bytes
    kernel->mailbox.buffer[1] = MBOX_REQUEST;
    // tags begin
    kernel->mailbox.buffer[2] = MBOX_TAG_GETREVISION;  // tag identifier
    kernel->mailbox.buffer[3] = 4;  // maximum of request and response value buffer's length.
    kernel->mailbox.buffer[4] = MBOX_TAG_REQUEST;
    kernel->mailbox.buffer[5] = 0;  // buffer for revision
    kernel->mailbox.buffer[6] = MBOX_TAG_END;
    // tags end
    Mailbox::ReturnCode ret = kernel->mailbox.mailboxExchange((uint8_t) MBOX_CH_PROP);
    if (ret) {
        kernel->printer.printf("Mailbox error: get_board_revision(), %s\n",
                               kernel->mailbox.errorCodeToStr(ret));
    } else {
        kernel->printer.printf("Board revision\t: 0x%X\n", kernel->mailbox.buffer[5]);
    }
}

void Shell::getArmMemoryInfo(void)
{
    kernel->mailbox.buffer[0] = 8 * 4;
    kernel->mailbox.buffer[1] = MBOX_REQUEST;
    // tags begin
    kernel->mailbox.buffer[2] = MBOX_TAG_GETMEMORY;
    kernel->mailbox.buffer[3] = 8;  // maximum of request and response value buffer's length.
    kernel->mailbox.buffer[4] = MBOX_TAG_REQUEST;
    kernel->mailbox.buffer[5] = 0;  // buffer for memory base address
    kernel->mailbox.buffer[6] = 0;  // buffer for memory size
    kernel->mailbox.buffer[7] = MBOX_TAG_END;
    // tags end

    Mailbox::ReturnCode ret = kernel->mailbox.mailboxExchange((uint8_t) MBOX_CH_PROP);
    if (ret) {
        kernel->printer.printf("Mailbox error: get_arm_memory_info(), %s\n",
                               kernel->mailbox.errorCodeToStr(ret));
    } else {
        kernel->printer.printf("ARM memory base\t: 0x%08X\n", kernel->mailbox.buffer[5]);
        kernel->printer.printf("ARM memory size\t: 0x%08X\n", kernel->mailbox.buffer[6]);
    }
}

void Shell::commandReboot()
{
    kernel->printer.printf("\n");
    power::powerReset(100);
}
