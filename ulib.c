/**********************************************
 * Author: Peter Higginbotham
 * The source code for the ulib user library, which interfaces with the OS
 */
#include "ulib.h"

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
static int syscall(SyscallArg_t* argument) {
    if (argument->call == HALT) {
        if (argument->buffer != NULL)
            return ERR;
    } else if (argument->call == PRINTS || argument->call == GETI 
            || argument->call == GETS) {
        if (argument->buffer == NULL || argument->size == 0)
            return ERR;
    } else {
        // Invalid call
        return ERR;
    }

    asm("TRAP");

    if (argument->status == RESULT_PENDING) {
        // Wait on the result - either argument->call or argument->status
        while(argument->call >= 0) 
        {}

        // Check if there were errors on an async IO action
        if (argument->call & 0x40000000) {
            argument->status = ERR;
        } else {
            argument->status = OK;
        }
    }
    
    return argument->status;
}

/******************************************************************************/
// Thread Safety: Safe
static int safe_strlen(char* string) {
    int bp;
    int lp;
    int maxLength;
    int length = 0;
    bp = asm2("PUSHREG", BP_REG);
    lp = asm2("PUSHREG", LP_REG);

    // The number of bytes between string and lp
    maxLength = lp - (int)(string + bp);

    while(maxLength > 0 && *string) {
        length++;
        string++;
        maxLength--;
    }

    if (maxLength <= 0)
        return -1;
    else
        return length;
}

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
int prints(char* string) {
    if (string == NULL)
        return ERR;
    
    SyscallArg_t arg;
    int stringLength;

    arg.call = PRINTS;
    arg.buffer = string;
    
    // Make sure that strlen doesn't go outside of memory bounds
    if ((stringLength = safe_strlen(string)) == -1)
        return ERR;
    
    arg.size = stringLength + 1;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************/
// Thread Safety: Safe
int printi(int value) {
    char buff[20];
    itostr(value, buff);

    return prints(buff);
}

/******************************************************************************/
// Thread Safety: Safe
int geti() {
    int val = 0;
    SyscallArg_t arg;
    arg.call = GETI;
    arg.buffer = (char*)&val;
    arg.size = sizeof(int);

    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return val;
}

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
int gets(char* buff) {
    SyscallArg_t arg;
    
    arg.call = GETS;
    arg.buffer = buff;
    arg.size = 256;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return OK;
}

int exit() {
    halt();
}

int exec(char* filename) {
    return prints(filename);
}

/******************************************************************************/
// Thread Safety: Unsafe
int halt() {
    SyscallArg_t arg;
    arg.call = HALT;
    arg.buffer = NULL;
    arg.size = 0;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;
    
    return OK;
}
