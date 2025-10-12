#ifndef LAB2_CPP_SHELL_HPP
#define LAB2_CPP_SHELL_HPP
#include "config.hpp"
#include "types.hpp"

class Shell final
{
public:
    Shell();
    void run();

private:
    enum ErrorCode {
        ERR_CODE_MSG_TOO_LONG = -1,
        ERR_CODE_TOO_LONG_TOKEN = -2,
        ERR_CODE_EMPTY_COMMAND = -3,
        ERR_CODE_TOO_MANY_TOKENS = -4,
    };

    static const char *const errorMessages[];

    char buffer[SHELL_BUFFER_MAX_SIZE + 1];
    uint32_t bufferUsed = 0;
    char tokens[SHELL_TOKEN_MAX_NUM][SHELL_TOKEN_MAX_LEN + 1];
    uint32_t tokensCount = 0;

    inline uint32_t bufferRemaining() const;
    inline const char *errorCodeToMessage(ErrorCode errorCode) const;

    void newLine();
    int32_t readCommand();
    int32_t parseCommand();
    void executeCommand();
    inline bool isHelp() const;

    void commandHelp();
    void commandHello();
    void commandMailbox();
    void commandReboot();

    void getBoardRevision(void);
    void getArmMemoryInfo(void);
};

#endif
