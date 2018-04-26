/**********************************************
 * Author: Peter Higginbotham
 * The source code for the ulib user library, which interfaces with the OS
 */
#include "ulib.h"

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
static int syscall(SyscallArg_t* argument) {
    if (argument->whichCall == HALT) {
        if (argument->argument != NULL)
            return ERR;
    } else if (argument->whichCall == PRINTS 
        || argument->whichCall == GETI 
        || argument->whichCall == GETS) {
        if (argument->argument == NULL)
            return ERR;
    } else {
        // Invalid call
        return ERR;
    }

    asm("TRAP");

    if (argument->status == RESULT_PENDING) {
        // Wait on the result
        while(argument->io.op >= 0) 
        {}

        argument->status = OK; 
    }
    
    return argument->status;
}

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
int prints(char* string) {
    SyscallArg_t arg;
    arg.whichCall = PRINTS;
    arg.argument = string;
    
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
    arg.whichCall = GETI;
    arg.argument = (char*)&val;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return val;
}

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
int gets(char* buff) {
    SyscallArg_t arg;
    
    arg.whichCall = GETS;
    arg.argument = buff;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************/
// Thread Safety: Unsafe
int halt() {
    SyscallArg_t arg;
    arg.whichCall = HALT;
    arg.argument = NULL;

    syscall(&arg);

    if (arg.status != OK)
        return ERR;
    
    return OK;
}
