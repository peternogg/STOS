#include <string.h>
#include <syscodes.h>
#include <machine_def.h>

#include "mymalloc.h"
#include "kerncommon.h"
#include "syscalls.h"
#include "sched.h"

#define PROC_MAX 20
static ProcessInfo_t g_ProcessTable[PROC_MAX];
static int g_CurrentTask;
static int g_RunningTasks;

static int findSlotWithStatus(int status, int start);
static int finalizeLoading(ProcessInfo_t* process);
static void findNextProcess(ProcessorState_t* context, char oldStatus, char newStatus);

/*******************************************************************************
 * Initializes the scheduler and sets the idle task to the context given
 * 
 * Preconditions:
 *  
 * Postconditions:
 *  g_ProcessTable is reset and any processes are 'killed'
 *  
 */
void sched_init() {
    int i;
    for(i = 1; i < PROC_MAX; i++) {
        g_ProcessTable[i].state = FREE_SLOT;
        g_ProcessTable[i].pname[0] = 0;
        memset(&g_ProcessTable[i].context, 0, sizeof(ProcessorState_t));
    }

    /* Set the idle process to the OS, and set up to idle */
    g_CurrentTask = 0;
    g_RunningTasks = 0;
    g_ProcessTable[0].state = RUNNING;
    g_ProcessTable[0].pname[0] = 'k';
    g_ProcessTable[0].pname[1] = 0;
}

int sched_userExec(SyscallArg_t* arg) {
    int slot = sched_exec(arg->buffer);
    if (slot > 0) {
        g_ProcessTable[slot].context.sp = (int)arg;
    }

    return slot;
}

/*******************************************************************************
 * Start a new process, with the filename given in the InpArg argument's param1
 * 
 * Preconditions:
 *  io is not NULL and contains a pointer to a filename
 * 
 * Postconditions:
 *  A new program begins loading into the machine if there is a free slot
 *  A slot in g_ProcessTable is modified and taken by the new program
 *  io->op is set to EXEC_CALL
 *  io->param2 is set to 0
 *  
 * Returns:
 *  NO_SLOTS_AVAILABLE(-1), OUT_OF_MEMORY(-2), or the selected slot on success. 
 * 
 * Thread safety: None
 */
int sched_exec(char* filename) {
    int slot;
    void* base;
    int limit;
    InpArg* io;

    /* Find an open process slot in the list */
    slot = findSlotWithStatus(FREE_SLOT, g_CurrentTask);
    
    if (slot == -1)
        return NO_SLOTS_AVAILABLE;

    /* Set the loading bounds for the new program */
    base = my_get_largest(&limit);
    if (base == NULL)
        return OUT_OF_MEMORY;

    /* Set up the new slot and borrow some of the fields of the context for the
    argument to INP */
    g_RunningTasks++;
    strncpy(g_ProcessTable[slot].pname, filename, PROG_NAME_LENGTH - 1);
    g_ProcessTable[slot].pname[PROG_NAME_LENGTH] = 0;
    g_ProcessTable[slot].state = LOADING;
    g_ProcessTable[slot].context.flag = FL_USER_MODE;
    g_ProcessTable[slot].context.bp = (int)base;
    // lp-ip-fp are contiguous words which aren't needed during loading
    // (lp is set later, when the load finishes)
    io = (InpArg*)&g_ProcessTable[slot].context.lp;
    io->op = EXEC_CALL;
    io->param1 = g_ProcessTable[slot].pname;
    io->param2 = 0;
    /* Set up the registers for inp */
    asm2("POPREG", BP_REG, base);
    asm2("POPREG", LP_REG, limit);
    /* Start loading the program */
    asm("INP", io);
    while(io->op >= 0) {}
    return slot;
}

static void dumpProcessBlock(int idx) {
    char buff[14];
    ProcessInfo_t* block = &g_ProcessTable[idx];
    asm("OUTS", "Block index ");
    asm("OUTS", itostr(idx, buff));
    asm("OUTS", "\nName: ");
    asm("OUTS", block->pname);
    asm("OUTS", "\nState: ");
    asm("OUTS", itostr(block->state, buff));
    asm("OUTS", "\nSP value: ");
    asm("OUTS", itostr(block->context.sp, buff));
    asm("OUTS", "\n== END BLOCK ==\n");
}

static void dumpPTable() {
    int i;
    asm("OUTS", "\n==== PTABLE DUMP ====\n");
    for (i = 0; i < PROC_MAX && g_ProcessTable[i].state != FREE_SLOT; i++) {
        dumpProcessBlock(i);
    }
    asm("OUTS", "\n==== END PTABLE DUMP ====\n");
}

/*******************************************************************************
 * Schedule the next runnable process on the processor for the next timeslice.
 * If a process is LOADING, but is done loading, then that process's loading is
 * finalized and that process is set to run.
 * 
 * Preconditions:
 *  context is not NULL, and is the state of the processor in the context of the
 *      process which was just preempted
 * 
 * Postconditions:
 *  The next ready process is set to run on the processor. If the selected slot
 *      was LOADING, then the scheduler finalizes the load and starts that slot.
 *  If no process is ready to run, then the scheduler will run the idle task in
 *      slot 0.
 * 
 * Returns:
 *      -1 if context is NULL.
 *      0 on success
 * 
 * Thread safety: None
 */
void sched_next(ProcessorState_t* context) {
    findNextProcess(context, READY, RUNNING);
}

void sched_exitCurrent(ProcessorState_t* context) {
    my_free(g_ProcessTable[g_CurrentTask].context.bp);
    g_RunningTasks--;
    if (g_RunningTasks <= 0)
        asm("HALT");
    findNextProcess(context, FREE_SLOT, RUNNING);
}

static void findNextProcess(ProcessorState_t* context, char oldStatus, char newStatus) {
    //dumpPTable();
    /* Find the next process which is runnable or loadable */
    //dumpPTable();
    int next = findSlotWithStatus(READY | LOADING, g_CurrentTask);
    //char buff[14];
    ProcessInfo_t* nextProcess = NULL;
    /* Save the old context */
    memcpy(&g_ProcessTable[g_CurrentTask].context, context, sizeof(*context));
    g_ProcessTable[g_CurrentTask].state = oldStatus;

    /* Ensure that we get a process which is actually runnable */
    do {
        if (next == -1) {
            /* The loop didn't find any runnable processes, so start idling */
            nextProcess = &g_ProcessTable[0];
            next = 0;
        } else {
            /* Found a ready slot to run */
            if (g_ProcessTable[next].state == LOADING) {
                if (finalizeLoading(&g_ProcessTable[next]) > 0) {
                    /* If the program wasn't actually ready, try again */
                    next = findSlotWithStatus(READY | LOADING, next);
                } else {
                    nextProcess = &g_ProcessTable[next];
                }
            } else
                nextProcess = &g_ProcessTable[next];
       }
    } while(nextProcess == NULL);

    memcpy(context, &nextProcess->context, sizeof(*context));
    nextProcess->state = newStatus;
    g_CurrentTask = next;
}

/*******************************************************************************
 * Scan g_ProcessTable for a slot which matches the given status bitmask. The
 * scan proceeds in queue order from start + 1 to start - 1 and ignores slot 0,
 * since that is the idle task
 * 
 * Preconditions:
 *  start is in [1, PROC_MAX]
 *  status is non-zero and is a bitmask of process states
 * 
 * Postconditions:
 *  None
 * 
 * Returns:
 *  -1 on error or if no slot was found which matched the status
 *  A slot number in [1, PROC_MAX] if a qualifying slot was found
 * 
 * * Thread safety: Possible race condition on g_ProcessTable entries
 */
static int findSlotWithStatus(int status, int start) {
    int slot = -1;
    int cand;
    /* Check the slots after the start */
    for (cand = start + 1; cand < PROC_MAX; cand++) {
        if (g_ProcessTable[cand].state & status) {
            slot = cand;
            cand = PROC_MAX;
        }
    }

    if (slot == -1) {
        /* Check the slots before the start, ignoring the OS in slot 0 */
        for (cand = 1; cand < start; cand++) {
            if (g_ProcessTable[cand].state & status) {
                slot = cand;
                cand = start;
            }
        }
    }

    return slot;
}

static int finalizeLoading(ProcessInfo_t* process) {
    // If the process is done loading, then finalize its settings. 
    // Otherwise, return 1
    int* stackSize;
    InpArg* io = &process->context.lp;

    if (io->op >= 0)
        return 1; // Not done loading

    // Notify userspace of the finished loading
    if (process->context.sp != NULL) {
        ((SyscallArg_t*)process->context.sp)->call = io->op;
        process->context.sp = 0;
    }

    if (io->op & 0x40000000) {
        return 2; // Error while loading
    }

    // Set the registers for the new process
    stackSize = io->param2;
    process->context.sp = (int)stackSize + 4 - process->context.bp;
    process->context.lp = (int)stackSize + *stackSize;
    process->context.fp = process->context.sp;
    process->context.ip = 8;

    // Trim memory
    my_set_limit((void*)process->context.bp, (void*)process->context.lp);
    
    process->state = READY;
    return 0;
}
