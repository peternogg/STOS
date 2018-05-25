/**********************************************
 * Author: Peter Higginbotham
 * The source code for the ulib user library, which interfaces with the OS
 */
#include "ulib.h"

#pragma startup ulib_startup
// User code forward declaration
int main();

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
static int syscall(SyscallArg_t* argument) {
    if (argument->call == HALT 
     || argument->call == EXIT 
     || argument->call == YIELD)
    {
        if (argument->buffer != NULL || argument->size != 0) {
            return ERR;
        }
    } else if (argument->call == PRINTS 
            || argument->call == GETI 
            || argument->call == GETS 
            || argument->call == EXEC)
    {
        if (argument->buffer == NULL || argument->size == 0) {
            return ERR;
        }
    } else if (argument->call == SLEEP) {
        if (argument->buffer != NULL || argument->size == 0)
            return ERR;
    } else {// Invalid call
        return ERR;
    }

    asm("TRAP");

    if (argument->status == RESULT_PENDING) {
        // Wait on the result - either argument->call or argument->status
        while(argument->call >= 0) 
        {
            yield(); // Give up the CPU instead of polling
        }

        // Check if there were errors on an async IO action
        if (argument->call & 0x40000000) {
            argument->status = ERR;
        } else {
            argument->status = OK;
        }
    }
    
    return argument->status;
}

void ulib_startup() {
    main();
    exit();
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

    if (syscall(&arg) == ERR || arg.status != OK)
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

    

    if (syscall(&arg) == ERR || arg.status != OK)
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

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

int exit() {
    SyscallArg_t arg;
    arg.call = EXIT;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
}

int exec(char* filename) {
    SyscallArg_t arg;
    arg.call = EXEC;
    arg.buffer = filename;
    arg.size = safe_strlen(filename);
    if (arg.size == -1)
        return ERR;

    arg.size += 1;
    
    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
}

/******************************************************************************/
// Thread Safety: Unsafe
int halt() {
    SyscallArg_t arg;
    arg.call = HALT;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
    
    return OK;
}

// Yields the CPU for another task
int yield() {
    SyscallArg_t arg;
    arg.call = YIELD;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

int sleep(int sleepTime) {
    SyscallArg_t arg;
    arg.call = SLEEP;
    arg.buffer = NULL;
    arg.size = sleepTime;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

int get_time() {
    SyscallArg_t arg;
    arg.call = GET_TIME;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
    
    return arg.size;
}
