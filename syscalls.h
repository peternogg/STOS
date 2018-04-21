#define PRINTS 0
#define GETI   1
#define GETS   2
#define HALT   3

const int OK = 0;
const int GENERAL_ERROR = 0x01;
const int NO_SUCH_CALL = 0x02;
const int INVALID_ARGUMENT = 0x04;
const int BAD_POINTER = 0x08;

typedef struct {
    int whichCall;
    char* argument;
    int status;
} SyscallArg_t;
