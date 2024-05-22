#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000

#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_MEMORY_INFO 0x00010005
#define POWER_PFF           0x00028001

extern volatile unsigned int mailbox[36];

int mailbox_call(unsigned char channel);