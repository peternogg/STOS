/**********************************************
 * Author: Peter Higginbotham
 * The source code a (cool, brand new) OS for the Stackl Interpreter
 */
#include <machine_def.h>
#include <syscodes.h>
#include <string.h>
#include <pio_term.h>
#include <timer.h>
#include "syscalls.h"

#pragma feature inp
#pragma startup startup

// IO Operation block
typedef struct {
    int op;
    char* param1;
    int param2;
} InpArg;

static int g_IVEC[16];

static void printString(SyscallArg_t* argument, int limit);
static void getString(SyscallArg_t* argument, int limit);
static void getInteger(SyscallArg_t* argument, int limit);
static void trap_handler(SyscallArg_t* argument);
static void systrap(SyscallArg_t* argument);
static void timerInterrupt();

/******************************************************************************/
// Thread Safety: None
void startup() {
    InpArg io_blk;

    int bp;
    int high_mem;
    int *stack_size;

    g_IVEC[1] = (int)systrap;
    g_IVEC[3] = (int)timerInterrupt;
    asm2("POPREG", IVEC_REG, g_IVEC);

    *((int*)TIMER_LIMIT) = 7000000;
    *((int*)TIMER_CSR) = 1;
    *((int*)TIMER_COUNT) = 0;

    // Set the BP leaving enough room for our stack (64 bytes)
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

/******************************************************************************/
// Thread Safety: Safe
static void systrap(SyscallArg_t* argument) {
    trap_handler(argument);
    asm("RTI");
}

static void timerInterrupt() {
    asm("OUTS", "a");
    asm("RTI");
}

/******************************************************************************/
// Thread Safety: Safe as long as string is not modified during execution
static int stringIsInvalid(char* string, int length, char* limit) {
    // Length includes null character, so -1 for that and -1 for array indexing
    return ((string + length) <= limit && string[length - 2] == 0);
}

/******************************************************************************/
// Thread Safety: Safe if BP, LP, and *argument are not changed during execution
static void trap_handler(SyscallArg_t* argument) {
    // Adjust the argument pointer
    int base;
    int limit;
    base = asm2("PUSHREG", BP_REG);
    limit = asm2("PUSHREG", LP_REG);

    argument = (SyscallArg_t*)((char*)argument + base);
    // Check the user's arguments
    // If it's outside their memory range, or the pointer inside it is outside
    // the range, then stop
    if (argument > limit)
        return;

    argument->status = OK;
    argument->buffer += base;
    // Don't let the user pass something outside their memory
    if (argument->buffer > limit)  {
        argument->status = BAD_POINTER;
        return;
    }

    if (argument->call == HALT)
        asm("HALT");
    else if (argument->call == PRINTS)
        printString(argument, limit);
    else if (argument->call == GETS)
        getString(argument, limit);
    else if (argument->call == GETI)
        getInteger(argument, limit);
    else
        argument->status = NO_SUCH_CALL;
}

static void printString(SyscallArg_t* argument, int limit) {
    // Make sure the user didn't send us an empty buffer
    if (argument->size == 0) {
        argument->status = INVALID_ARGUMENT;
        return;
    }
    // Make sure there's a null pointer before LP
    if (stringIsInvalid(argument->buffer, argument->size, limit)) {
        argument->status = INVALID_ARGUMENT;
        return;
    }
    // Start the output stage
    argument->call = PRINTS_CALL;
    argument->size = 0;
    argument->status = RESULT_PENDING;
    asm("INP", argument);
}
static void getString(SyscallArg_t* argument, int limit) {
    // There must be 256 bytes between argument and limit
    if (argument->buffer + 256 >= limit) {
        argument->status = BAD_POINTER;
        return;
    }

    argument->call = GETL_CALL;
    argument->size = 0;
    argument->status = RESULT_PENDING;
    asm("INP", argument);
}

static void getInteger(SyscallArg_t* argument, int limit) {
    // There must be at least an int worth of space
    if (argument->buffer + sizeof(int) >= limit) {
        argument->status = BAD_POINTER;
        return;
    }

    argument->call = GETI_CALL;
    argument->size = 0;

    argument->status = RESULT_PENDING;
    asm("INP", argument); // Reuse argument as IO block
}
