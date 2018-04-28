/**********************************************
 * Author: Peter Higginbotham
 * The source code a (cool, brand new) OS for the Stackl Interpreter
 */
#include <machine_def.h>
#include <syscodes.h>
#include <string.h>
#include <pio_term.h>
#include "syscalls.h"

#pragma feature inp
#pragma feature pio_term
#pragma startup startup
#pragma systrap systrap
#pragma interrupt ISR

// IO Operation block
typedef struct {
    int op;
    char* param1;
    int param2;
} InpArg;

SyscallArg_t* g_currentString;

/******************************************************************************/
// Thread Safety: None
void startup() {
    InpArg io_blk;

    int bp;
    int high_mem;
    int *stack_size;

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
// Thread Safety: Safe as long as string is not modified during execution
static int stringUnderLimit(char* string, int length, char* limit) {
    // Length includes null character, so -1 for that and -1 for array indexing
    return ((string + length) < limit && string[length - 2] == 0);
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

    if (argument->call == HALT) {
        asm("HALT");
    } else if (argument->call == PRINTS) {
        // Make sure there's a null pointer before LP
        if (stringUnderLimit(argument->buffer, argument->size, limit)) {
            argument->status = INVALID_ARGUMENT;
            return;
        }

        //argument->status = RESULT_PENDING;
        asm("OUTS", argument->buffer);
        //*((int*)PIO_T_IE_XMIT) = *(argument->buffer);
    } else if (argument->call == GETS) {
        // There must be 256 bytes between argument and limit
        if (argument->buffer + 256 >= limit) {
            argument->status = BAD_POINTER;
            return;
        }

        argument->call = GETL_CALL;
        argument->size = 0;
        argument->status = RESULT_PENDING;
        asm("INP", argument);
    } else if (argument->call == GETI) {
        // There must be at least an int worth of space
        if (argument->buffer + sizeof(int) >= limit) {
            argument->status = BAD_POINTER;
            return;
        }

        argument->call = GETI_CALL;
        argument->size = 0;

        argument->status = RESULT_PENDING;
        asm("INP", argument); // Reuse argument as IO block
    } else {
        argument->status = NO_SUCH_CALL;
    }
}

/******************************************************************************/
// Thread Safety: Safe
void systrap(SyscallArg_t* argument) {
    trap_handler(argument);
    asm("RTI");
}

void ISR() {
    asm("RTI");
}
