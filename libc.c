#include "libc.h"

static int syscall(int call, char* argument) {
    int status = 0;

    if (call == HALT) {
        if (argument != NULL)
            return ERR;
    } else if (call == PRINTS || call == GETI || call == GETS) {
        if (argument == NULL)
            return ERR;
    }

    asm("TRAP"); 
    return 0;
}

int prints(char* string) {
    syscall(PRINTS, string);
}

int printi(int value) {
    char buff[14]; // 10 digits + commas + null terminator
    itostr(value, buff);

    return syscall(PRINTS, buff);
}

int geti() {
    int val = -1;

    if (syscall(GETI, (char*)&val) == ERR)
        return ERR;
    else
        return val;
}

int gets(char* buff) {
    if (buff == NULL)
        return -1;
    
    return syscall(GETS, buff);
}

int halt() {
    syscall(HALT, NULL);
}
