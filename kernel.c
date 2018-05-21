/**********************************************
 * Author: Peter Higginbotham
 * The source code a (cool, brand new) OS for the Stackl Interpreter
 */
#include <machine_def.h>
#include <syscodes.h>
#include <string.h>
#include <timer.h>

#include "kerncommon.h"
#include "sched.h"
#include "mymalloc.h"
#include "syscalls.h"

#pragma feature inp
#pragma startup startup

#define TIMER_INTERVAL 140000 // instructions
#define OS_STACK_SIZE 512
#define MEMORY_END_PAD 16

static int g_IVEC[16];

static void printString(SyscallArg_t* argument, int limit);
static void getString(SyscallArg_t* argument, int limit);
static void getInteger(SyscallArg_t* argument, int limit);
static void startNewProgram(SyscallArg_t* argument, int limit);
static void exitCurrentProgram();
static void trapHandler(SyscallArg_t* argument);
static void systrap(SyscallArg_t* argument);
static void timerInterrupt();

/******************************************************************************/
// Thread Safety: None
void startup() {
    int lowLimit;
    int highLimit;
    int memorySize;

    // Initialize interrupts and preemption timer
    g_IVEC[1] = (int)systrap;
    g_IVEC[3] = (int)timerInterrupt;
    asm2("POPREG", IVEC_REG, g_IVEC); 

    // Initialize the memory manager
    lowLimit = asm2("PUSHREG", SP_REG); // OS Stack pointer
    highLimit = asm2("PUSHREG", LP_REG); // Address of the end of memory

    lowLimit += OS_STACK_SIZE;
    memorySize = (highLimit - lowLimit) - MEMORY_END_PAD;

    my_mem_init((void*)lowLimit, memorySize);

    sched_init();
    sched_exec("user1.slb");

    *((int*)TIMER_LIMIT) = TIMER_INTERVAL;
    *((int*)TIMER_CSR) = 1;
    *((int*)TIMER_COUNT) = TIMER_INTERVAL - 10;

    while(1) { }

    asm("HALT");
}
/* The interrupt frame whenever the kernel is activated */
static ProcessorState_t* state;
/******************************************************************************/
// Thread Safety: Safe
static void systrap(SyscallArg_t* argument) {
    state = asm2("PUSHREG", SP_REG);
    state--;

    trapHandler(argument);
    asm("RTI");
}

static void timerInterrupt() {
    state = asm2("PUSHREG", FP_REG);
    state--;

    //asm("OUTS", "\nwhop\n");
    sched_next(state);
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
static void trapHandler(SyscallArg_t* argument) {
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

    argument->buffer += base;
    // Don't let the user pass something outside their memory
    if (argument->buffer > limit)  {
        argument->status = BAD_POINTER;
        return;
    }

    // if (argument->call == HALT)
    //     asm("HALT");
    // else
    if (argument->call == PRINTS)
        printString(argument, limit);
    else if (argument->call == GETS)
        getString(argument, limit);
    else if (argument->call == GETI)
        getInteger(argument, limit);
    else if (argument->call == EXIT)
        exitCurrentProgram();
    else if (argument->call == EXEC)
        startNewProgram(argument, limit);
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

static void startNewProgram(SyscallArg_t* argument, int limit) {
    if (argument->buffer == 0 || argument->size == 0) {
        argument->status = INVALID_ARGUMENT;
        return;
    }

    if (stringIsInvalid(argument->buffer, argument->size, limit)) {
        argument->status = BAD_POINTER;
        return;
    }

    sched_exec(argument->buffer);
}

static void exitCurrentProgram() {
    sched_exitCurrent(state);
}
