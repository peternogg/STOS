#include "libc.h"

static int syscall(SyscallArg_t* argument) {
    if (argument->whichCall == HALT) {
        if (argument->argument != NULL)
            return ERR;
    } else if (argument->whichCall == PRINTS 
        || argument->whichCall == GETI 
        || argument->whichCall == GETS) {
        if (argument->argument == NULL)
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

int prints(char* string) {
    SyscallArg_t arg;
    arg.whichCall = PRINTS;
    arg.argument = string;
    
    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return 0;
}

int printi(int value) {
    char buff[14]; // 10 digits + commas + null terminator
    SyscallArg_t arg;
    itostr(value, buff);

    arg.whichCall = PRINTS;
    arg.argument = buff;
    
    syscall(&arg);

    if (arg.status != OK)
        return ERR;

    return 0;
}

int geti() {
    int val = -1;
    SyscallArg_t arg;
    arg.whichCall = GETI;
    arg.argument = (char*)&val;

    syscall(&arg);

    if (arg.status != OK)
        return -1;

    return val;
}

int gets(char* buff) {
    SyscallArg_t arg;
    if (buff == NULL)
        return -1;
    
    arg.whichCall = GETS;
    arg.argument = buff;

    syscall(&arg);

    if (arg.status != OK)
        return -1;

    return 0;
}

int halt() {
    SyscallArg_t arg;
    arg.whichCall = HALT;
    arg.argument = NULL;

    syscall(&arg);

    if (arg.status != OK)
        return -1;
    
    return 0;
}
