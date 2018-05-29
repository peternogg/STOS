/************************
 * Author: Peter Higginbotham 
 * Kernel/Syscalls external interface definition
 */

#pragma once

// Syscall numbers
#define PRINTS     0
#define GETI       1
#define GETS       2
#define HALT       3
#define EXIT       4
#define EXEC       5
#define YIELD      6
#define SLEEP      7
#define GET_TIME   8
#define GET_PID    9
#define GET_PPID  10
#define WAIT      11


// Errors and information (For SyscallArg_t.status)
const int RESULT_PENDING     = -0x02;
const int DONE               = -0x03;
const int OK                 = 0;
const int GENERAL_ERROR      = 0x01;
const int NO_SUCH_CALL       = 0x02;
const int INVALID_ARGUMENT   = 0x04;
const int BAD_POINTER        = 0x08;

// Syscall block
typedef struct {
    int call;
    char* buffer;
    int size;
    int status;
} SyscallArg_t;
