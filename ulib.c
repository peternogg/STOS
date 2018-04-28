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
        while(argument->call >= 0 && argument->status == RESULT_PENDING) 
        {}

        argument->status = OK;
    }
    
    return argument->status;
}

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
int prints(char* string) {
    if (string == NULL)
        return ERR;
        
    SyscallArg_t arg;
    arg.call = PRINTS;
    arg.buffer = string;
    arg.size = strlen(string) + 1;
    
    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************/
// Thread Safety: Safe
int printi(int value) {
    char buff[20];
    SyscallArg_t arg;
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
