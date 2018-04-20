#include <machine_def.h>
#include <syscodes.h>
#include "syscalls.h"

#pragma feature inp
#pragma startup startup
#pragma systrap systrap

typedef struct {
    int op;
    char* param1;
    int param2;
} InpArg;

void startup() {
    InpArg io_blk;

    int bp;
    int high_mem;
    int *stack_size;

    // Set the BP leaving enough room for our stack
    bp = asm2("PUSHREG", SP_REG);
    bp += 64;
    asm2("POPREG", BP_REG, bp);

    // Load user.slb into memory
    io_blk.op = EXEC_CALL;
    io_blk.param1 = (int)"user.slb";
    io_blk.param2= 0;
    asm("INP", &io_blk);
    while (io_blk.op >= 0)
    {
    }

    // Set the LP leaving 1000 bytes of stack space
    stack_size = io_blk.param2;
    high_mem = io_blk.param2 + *stack_size;
    asm2("POPREG", LP_REG, high_mem);

    // Set SP and FP
    // NOTE: FP must be set LAST!
    high_mem = io_blk.param2 + 4 - bp;
    asm("DUP", high_mem);
    asm2("POPREG", FP_REG);
    asm2("POPREG", SP_REG);

    // Execute user.slb
    asm2("JMPUSER", 8); 
    asm("HALT");
}

static int validate_args(int call, char* argument) {

}

static int trap_handler(int call, char* argument) {
    InpArg inpArg;
    inpArg.op = 0;
    inpArg.param1 = 0;
    inpArg.param2 = 0;

    // Adjust the user mode pointer
    int base;
    base = asm2("PUSHREG", BP_REG);
    argument += base;

    if (!validate_args(call, argument))
        return -1;

    if (call == HALT) {
        asm("HALT");
    } else if (call == PRINTS) {
        asm("OUTS", argument);
    } else if (call == GETS) {
        inpArg.op = GETS_CALL;
        inpArg.param1 = argument;
        asm("INP", &inpArg);
        while(inpArg.op >= 0) {}
    } else if (call == GETI) {
        inpArg.op = GETI_CALL;
        inpArg.param1 = argument;
        asm("INP", &inpArg);
        while(inpArg.op >= 0) {}
    }
}

int systrap(int call, char* argument) {
    trap_handler(call, argument);
    asm("RTI");
}
