#include <machine_def.h>
#include "syscalls.h"

#pragma feature inp

typedef struct {
    int op;
    int param1;
    int param2;
} InpArg;

static int trap_handler(int call, int arg1, char* arg2, int* arg3) {
    InpArg arg;
    arg.op = 3;
    arg.param1 = (int)"trap_handler called\n";
    arg.param2 = 0;

    asm("INP", &arg);
    while(arg.op >= 0) {}

    if (call == HALT)
    {
        arg.param1 = (int)"halting!\n";
        asm("INP", &arg);
        while(arg.op >= 0) {}
    } else if (call == PRINTS) {
        arg.param1 = (int)"Called prints\n";
        asm("INP", &arg);
        while(arg.op >= 0) {}
    }
}

#pragma systrap systrap
int systrap(int call, int arg1, char* arg2, int* arg3) {
    trap_handler(call, arg1, arg2, arg3);
    asm("RTI");
}
