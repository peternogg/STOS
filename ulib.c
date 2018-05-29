/**********************************************
 * Author: Peter Higginbotham
 * The source code for the ulib user library, which interfaces with the OS
 */
#include "ulib.h"

#pragma startup ulib_startup
int main(); // User code forward declaration

/******************************************************************************/
// Thread Safety: Safe if argument is not shared between threads
static int syscall(SyscallArg_t* argument) {
    if (argument->call == HALT 
     || argument->call == EXIT 
     || argument->call == YIELD
     || argument->call == GET_PID
     || argument->call == GET_PPID)
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
    } else if (argument->call == SLEEP
            || argument->call == WAIT) {
        if (argument->buffer != NULL || argument->size == 0)
            return ERR;
    } else {
        // Invalid call
        return ERR;
    }

    asm("TRAP");

    if (argument->status == RESULT_PENDING) {
        // Wait on the result - either argument->call or argument->status
        // while(argument->call >= 0) 
        // {
        //     yield(); // Give up the CPU instead of polling
        // }

        // Check if there were errors on an async IO action
        if (argument->call & 0x40000000) {
            argument->status = ERR;
        } else {
            argument->status = OK;
        }
    }
    
    return argument->status;
}

/******************************************************************************
 * Wraps a usermode program's startup function to force it to exit when it
 * finishes.
 * Thread Safety: None
 */ 
void ulib_startup() {
    main();
    exit();
}

/******************************************************************************
* Count the number of characters in a string without exceeding the process's LP
* Thread Safety: Safe if string is not modified
*/
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

/******************************************************************************
 * Print a string to the console
 * Thread Safety: Safe if argument is not shared between threads
 */
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

/******************************************************************************
 * Print an integer to the screen
 * Thread Safety: Safe
 */
int printi(int value) {
    char buff[20];
    itostr(value, buff);

    return prints(buff);
}

/*******************************************************************************
 * Get an integer value from the user
 * Thread Safety: Safe
 * Multitasking Safety: None
 */
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

/******************************************************************************* 
 * Get a string from the user
 * Thread Safety: Safe if argument is not shared between threads
 * Multitasking Safety: None
 */
int gets(char* buff) {
    SyscallArg_t arg;
    
    arg.call = GETS;
    arg.buffer = buff;
    arg.size = 256;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************* 
 * Get the current process's PID
 * Thread Safety: Safe
 */
int getpid() {
    SyscallArg_t arg;
    arg.call = GET_PID;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return arg.size;
}

/******************************************************************************* 
 * Get the current process's parent's PID
 * Thread Safety: Safe
 */
int getppid() {
    SyscallArg_t arg;
    arg.call = GET_PPID;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return arg.size;
}

/******************************************************************************* 
 * Exit the current process
 * Thread Safety: None
 */
int exit() {
    SyscallArg_t arg;
    arg.call = EXIT;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
}

/******************************************************************************* 
 * Start a new process in the system with a given filename
 * Thread Safety: Safe
 */
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

    return arg.size;
}

/******************************************************************************* 
 * Shut down the system immediately.
 * Thread Safety: None
 */
int halt() {
    SyscallArg_t arg;
    arg.call = HALT;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
    
    return OK;
}

/******************************************************************************* 
 * Give up the rest of the current timeslice for this process.
 * Thread Safety: Safe
 */
int yield() {
    SyscallArg_t arg;
    arg.call = YIELD;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************* 
 * Set the current process to sleep for a number of instructions
 * Thread Safety: Safe
 */
int sleep(int sleepTime) {
    if (sleepTime < 0)
        return ERR;

    SyscallArg_t arg;
    arg.call = SLEEP;
    arg.buffer = NULL;
    arg.size = sleepTime;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;

    return OK;
}

/******************************************************************************* 
 * Get the current 'time' on the system (the number of instructions since start)
 * Thread Safety: Safe
 */
int get_time() {
    SyscallArg_t arg;
    arg.call = GET_TIME;
    arg.buffer = NULL;
    arg.size = 0;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
    
    return arg.size;
}

/******************************************************************************* 
 * Wait for a child process, as given by the PID argument
 * Thread Safety: Safe
 */
int wait(int pid) {
    SyscallArg_t arg;
    arg.call = WAIT;
    arg.buffer = NULL;
    arg.size = pid;

    if (syscall(&arg) == ERR || arg.status != OK)
        return ERR;
    
    return OK;
}
