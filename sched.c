/************************
 * Author: Peter Higginbotham 
 * The scheduling subsystem for StacklOS
 */

#include "sched.h"

#define PROC_MAX 20
static ProcessInfo_t g_ProcessTable[PROC_MAX];
static ProcessInfo_t* g_CurrentProcess;
static queue_t g_ReadyQueue;
static int g_LiveProcessCount;

static ProcessInfo_t* findNextReady();
static int findSlotWithStatus(char status, int start);
static int finalizeLoading(ProcessInfo_t* process);
static void deleteProcess(ProcessInfo_t* process);
static void wakeIOs();

static void dumpPTable();
static void dumpProcessBlock(int idx);
static void dumpProcessInfo(ProcessInfo_t* process);

/*******************************************************************************
 * Initializes the scheduler and sets the idle task to the context given
 * 
 * Preconditions:
 *  
 * Postconditions:
 *  g_ProcessTable is reset and any processes are 'killed'
 *  
 * Thread Safety: None
 */
void sched_init() {
    int i;
    for(i = 0; i < PROC_MAX; i++) {
        memset(&g_ProcessTable[i], 0, sizeof(ProcessInfo_t));
        g_ProcessTable[i].state = FREE_SLOT;
    }

    g_ReadyQueue = Q_Init(PROC_MAX);
    g_LiveProcessCount = 0;

    /* Set the idle process to the OS, and set up to idle */
    g_CurrentProcess = &g_ProcessTable[0];
    g_ProcessTable[0].state = RUNNING;
    memcpy(&g_ProcessTable[0].pname, "kernel", 7);
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

    if (filename == NULL || *filename == 0)
        return BAD_FILE_NAME;

    /* Find an open process slot in the list */
    slot = findSlotWithStatus(FREE_SLOT, 0);
    
    if (slot == -1)
        return NO_SLOTS_AVAILABLE;

    /* Set the loading bounds for the new program */
    base = my_get_largest(&limit);
    if (base == NULL) {
        return OUT_OF_MEMORY;
    }

    /* Set up the new slot and borrow some of the fields of the context for the
    argument to INP */
    strncpy(g_ProcessTable[slot].pname, filename, PROG_NAME_LENGTH - 1);
    g_ProcessTable[slot].pname[PROG_NAME_LENGTH] = 0;
    g_ProcessTable[slot].state = LOADING;
    g_ProcessTable[slot].context.flag = FL_USER_MODE;
    g_ProcessTable[slot].context.bp = (int)base;
    g_ProcessTable[slot].parent = sched_getPID();
    g_ProcessTable[slot].waitedOnBy = 0;
    
    g_ProcessTable[slot].currentIO = (InpArg*)&g_ProcessTable[slot].context.lp;
    g_ProcessTable[slot].currentIO->op = EXEC_CALL;
    g_ProcessTable[slot].currentIO->param1 = filename;
    g_ProcessTable[slot].currentIO->param2 = 0;

    /* Set up the registers for inp */
    asm2("POPREG", BP_REG, base);
    asm2("POPREG", LP_REG, limit);
    /* Start loading the program */
    asm("INP", g_ProcessTable[slot].currentIO);
    Q_Enqueue(g_ReadyQueue, &g_ProcessTable[slot]);
    g_LiveProcessCount++;

    return slot;
}

/*******************************************************************************
 * Display information about a particular ProcessInfo_t structure. May affect
 * timing as it uses a lot of OUTS instructions
 * 
 * Preconditions:
 *  None
 * Postconditions:
 *  Displays, using OUTS instructions, the information in a PCB
 *  
 * Thread Safety: Race on block argument
 */
static void dumpProcessInfo(ProcessInfo_t* block) {
    char buff[14];
    asm("OUTS", "\nAddress: 0x");
    asm("OUTS", xtostr((int)block, buff));
    asm("OUTS", "\nName: ");
    asm("OUTS", block->pname);
    asm("OUTS", "\nParent: ");
    asm("OUTS", itostr(block->parent, buff));
    asm("OUTS", "\nState: 0x");
    asm("OUTS", xtostr(block->state, buff));
    asm("OUTS", "\nWaited on by: ");
    asm("OUTS", itostr(block->waitedOnBy, buff));
    //if (block->state == DOING_IO) {
        asm("OUTS", "\nDoing IO: ");
        if (block->currentIO->op != 0)
            asm("OUTS", xtostr(block->currentIO->op, buff));
        else
            asm("OUTS", "No IO call registered?");
    //}
    if (block->state == SLEEPING) {
        asm("OUTS", "\nWakes at: ");
        asm("OUTS", itostr(block->wakeAt, buff));
        asm("OUTS", " (it is currently ");
        asm("OUTS", itostr(*(int*)TIMER_TIME, buff));
        asm("OUTS", ")");
    }
    asm("OUTS", "\nContext: ");
    asm("OUTS", "\n-> SP = 0x");
    asm("OUTS", xtostr(block->context.sp, buff));
    asm("OUTS", "\n-> FLAGS = 0x");
    asm("OUTS", xtostr(block->context.flag, buff));
    asm("OUTS", "\n-> BP = 0x");
    asm("OUTS", xtostr(block->context.bp, buff));
    asm("OUTS", "\n-> LP = 0x");
    asm("OUTS", xtostr(block->context.lp, buff));
    asm("OUTS", "\n-> IP = 0x");
    asm("OUTS", xtostr(block->context.ip, buff));
    asm("OUTS", "\n-> FP = 0x");
    asm("OUTS", xtostr(block->context.fp, buff));
    asm("OUTS", "\n== END BLOCK ==\n");
}

/*******************************************************************************
 * Display information about a PCB in the process table. Will ignore free slots.
 * 
 * Preconditions:
 *  idx references a Process block in the table which is not a FREE_SLOT
 *  idx is within the bounds of the process table. This function does not do
 *  bounds checking.
 * Postconditions:
 *  Displays information about the process
 * 
 * Arguments:
 *  The index of a PCB in the table
 *  
 * Thread Safety: Race on reference PCB
 */
static void dumpProcessBlock(int idx) {
    ProcessInfo_t* block = &g_ProcessTable[idx];
    if (block->state != FREE_SLOT)
        dumpProcessInfo(block);
}

/*******************************************************************************
 * Displays information about all the entries in the Process Table which are not
 * FREE_SLOTs.
 * 
 * Preconditions:
 *  None
 * Postconditions:
 *  Displays information about the process table
 *  
 * Thread Safety: Race on process table
 */
static void dumpPTable() {
    int i;
    asm("OUTS", "\n==== PTABLE DUMP ====\n");
    for (i = 0; i < PROC_MAX; i++) {
        dumpProcessBlock(i);
    }
    asm("OUTS", "Current program information: \n");
    dumpProcessInfo(g_CurrentProcess);

    asm("OUTS", "\n==== END PTABLE DUMP ====\n");
}

/*******************************************************************************
 * Switch to the next process in the ready queue and set the state of the old
 * (switched away from) process to old, and the state of the new (switched to)
 * process to new.
 * 
 * Preconditions:
 *  g_CurrentProcess is a valid process
 *  context is a pointer to a valid interrupt call frame
 *  
 * Postconditions:
 *  Switches from the current process to a different process which is ready.
 *  This process may be the OS if no other process is ready to run, or it may
 *  be the same process if it is the only process ready to run.
 *  
 * Thread Safety: None
 */
static void nextWithStates(ProcessorState_t* context, int old, int new) {
    memcpy(&g_CurrentProcess->context, context, sizeof(ProcessorState_t));
    g_CurrentProcess->state = old;
    g_CurrentProcess = findNextReady();
    g_CurrentProcess->state = new;
    memcpy(context, &g_CurrentProcess->context, sizeof(ProcessorState_t));   
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
 *  Any processes whose IO processes completed are woken up and placed in the
 *      ready queue
 * 
 * Thread safety: None
 */
void sched_next(ProcessorState_t* context) {
    if (g_CurrentProcess != &g_ProcessTable[0])
        Q_Enqueue(g_ReadyQueue, g_CurrentProcess);

    wakeIOs();

    nextWithStates(context, READY, RUNNING);
}

/*******************************************************************************
 * Exits the current process and cleans up its memory if applicable.
 * 
 * Preconditions:
 *  context is not null and points to a valid interrupt call frame
 * Postconditions:
 *  The process which called this function is freed and its slot is made into
 *      a FREE_SLOT.
 *  A new process is selected to run on the machine
 *  The count of live processes is decremented, and if it is 0, then the machine
 *      halts.
 *  
 * Thread Safety: None
 */
void sched_exitCurrent(ProcessorState_t* context) {
    if (g_CurrentProcess->waitedOnBy != 0) {
        // Wake up whatever is waiting on the process
        ProcessInfo_t* proc = &g_ProcessTable[g_CurrentProcess->waitedOnBy];

        proc->state = READY;
        Q_Enqueue(g_ReadyQueue, proc);
        deleteProcess(g_CurrentProcess);
    } else {
        if (g_CurrentProcess->parent != 0)
            g_CurrentProcess->state = ZOMBIE;
        else
            deleteProcess(g_CurrentProcess);
    }
    
    if (g_LiveProcessCount <= 0) {
        asm("OUTS", "OS Shutting Down\n");
        asm("HALT");
    }

    nextWithStates(context, g_CurrentProcess->state, RUNNING);
}


/*******************************************************************************
 * Puts the current process to sleep
 * 
 * Preconditions:
 *  Context is not null and references a valid interrupt call frame
 *  wakeAt is non-negative.
 * Postconditions:
 *  The current process is put to sleep and will wake up in the next timeslice
 *      after wakeAt instructions have passed.
 *  A new process is selected to run on the machine.
 *  
 * Thread Safety: None
 */
void sched_sleepCurrent(ProcessorState_t* context, int wakeAt) {
    if (wakeAt < 0)
        return;

    g_CurrentProcess->wakeAt = *((int*)TIMER_TIME) + wakeAt;
    Q_Enqueue(g_ReadyQueue, g_CurrentProcess);
    nextWithStates(context, SLEEPING, RUNNING);
}

/*******************************************************************************
 * Sets a process to wait on another process and block until the waited on
 * process has exited.
 * 
 * Preconditions:
 *  Context points to a valid interrupt call frame
 *  waitPID is a valid PID, and the process it references is alive or a zombie
 * Postconditions:
 *  The current process is dequeued and blocks until the process it is waiting
 *      on dies.
 *   
 * 
 * Returns:
 *  0 when successfully waiting. -1 on errors.
 *  
 * Thread Safety: None
 */
int sched_waitOn(ProcessorState_t* context, int waitPID) {    
    // PID outside of table?
    if (waitPID < 1 || waitPID > PROC_MAX) {
        return -1;
    }

    // Is the PID alive?
    if (g_ProcessTable[waitPID].state == FREE_SLOT)
        return -1;

    if (g_ProcessTable[waitPID].state == ZOMBIE) {
        // The process is already dead, so we need to collect it
        deleteProcess(&g_ProcessTable[waitPID]);
    } else {
        g_ProcessTable[waitPID].waitedOnBy = sched_getPID();
        nextWithStates(context, WAITING, RUNNING);
    }

    return 0;
}

/*******************************************************************************
 * Notify the scheduler that the current process is beginning an IO operation
 * 
 * Preconditions:
 *  Context points to a valid interrupt call frame
 *  g_CurrentProcess is a valid process which is executing an IO
 *  ioInfo points to the IO argument block which is being operated on
 * Postconditions:
 *  The current process's state is changed to DOING_IO and that process blocks
 *      until its IO is done.
 *  
 * Thread Safety: None
 */
void sched_BeginIO(ProcessorState_t* context, InpArg* ioInfo) {
    g_CurrentProcess->currentIO = ioInfo;
    nextWithStates(context, DOING_IO, RUNNING);
}

/*******************************************************************************
 * Returns the PID of the current process
 * 
 * Preconditions:
 *  g_CurrentProcess points within the process table
 * Postconditions:
 *  None
 *  
 * Returns:
 *  The PID of g_CurrentProcess. If g_CurrentProcess is invalid, this may be 
 *      nonsense.
 * Thread Safety: None
 */
int sched_getPID() {
    return (g_CurrentProcess - &g_ProcessTable[0]) / sizeof(ProcessInfo_t);
}

/*******************************************************************************
 * Returns the PID of the parent of g_CurrentProcess
 * 
 * Preconditions:
 *  g_CurrentProcess is a valid process
 * Postconditions:
 *  None
 *  
 * Returns:
 *  The PID of the parent of g_CurrentProcess
 * 
 * Thread Safety: None
 */
int sched_getPPID() {
    return g_CurrentProcess->parent;
}


/*******************************************************************************
 * Dequeue a process from the ready queue and ensure that it is ready to execute
 * The runtime of this function depends on the state of the ready queue
 * 
 * Preconditions:
 *  g_ReadyQueue is a valid queue and only contains things which are READY,
 *      LOADING, or SLEEPING, and only contains pointers to items in the 
 *      process table.
 * Postconditions:
 *  If the next process was loading, and it is finished loading, then it is 
 *      readyied for execution as in finalizeLoading(), and then the process
 *      is started.
 *  If the next process was sleeping, and its wake time has passed, then it is
 *      woken up and begins running.
 *  Note: If no processes are ready to run, this may make a full trip through
 *      the ready queue and dequeue/requeue everything.
 * 
 * Returns: 
 *  A valid Process, or PCB for the Idle process (the OS) if none are ready.
 *  
 * Thread Safety: None
 */
static ProcessInfo_t* findNextReady() {
    ProcessInfo_t* next = NULL;
    int result;
    int readyCount = Q_Elements(g_ReadyQueue);

    // Try to find a process which is ready to run. If there's none that are 
    // ready, then fall out of the loop and start the OS
    while(readyCount > 0 && next == NULL) {
        next = (ProcessInfo_t*)Q_Dequeue(g_ReadyQueue);
        if (next->state == LOADING) {
            result = finalizeLoading(next);
            if (result == 2) {
                // Error while loading - Clean up the process
                deleteProcess(next);

                next = NULL;
                readyCount--;
            } else if (result == 1) {
                // Not fully loaded yet
                Q_Enqueue(g_ReadyQueue, next);

                next = NULL;
                readyCount--;
            }
        } else if (next->state == SLEEPING) {
            if (*((int*)TIMER_TIME) < next->wakeAt) {
                Q_Enqueue(g_ReadyQueue, next);
                next = NULL;
                readyCount--;
            }
        }
    } 

    if (readyCount <= 0)
        return &g_ProcessTable[0];

    return next;
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
 * Thread safety: Possible race condition on g_ProcessTable entries
 */
static int findSlotWithStatus(char status, int start) {
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

/*******************************************************************************
 * Checks and finalizes the loading of a process
 * 
 * Preconditions:
 *  process is a non-null pointer to a process whose state is LOADING
 * Postconditions:
 *  The process's loading is finished and its initial registers are set.
 *  Its parent is notified of the finished load and can be woken up later.
 *  If there was an error while loading the program, then the process is not
 *      altered, and the function returns 2.
 *  If the process is not finished loading yet, then nothing is changed.
 * 
 * Returns:
 *  0 on success.
 *  1 if the process isn't yet done loading
 *  2 if there was an error while loading the program
 *  
 * Thread Safety: None
 */
static int finalizeLoading(ProcessInfo_t* process) {
    // If the process is done loading, then finalize its settings. 
    // Otherwise, return 1
    //dumpPTable();
    char buff[14];
    int* stackSize;

    if (process->currentIO->op >= 0)
        return 1; // Not done loading

    // Notify the parent that the program finished loading
    if (process->parent != 0) {
        g_ProcessTable[process->parent].currentIO->op = process->currentIO->op;
    }

    if (process->currentIO->op & 0x40000000) {
        return 2; // Error while loading
    }
    
    // Set the registers for the new process
    stackSize = process->currentIO->param2;
    process->context.sp = (int)stackSize + 4 - process->context.bp;
    process->context.lp = (int)stackSize + *stackSize;
    process->context.fp = process->context.sp;
    process->context.ip = 8;

    // Trim memory
    my_set_limit((void*)process->context.bp, (void*)process->context.lp);
    
    process->state = READY;
    return 0;
}

/*******************************************************************************
 * Transform a process into a free slot
 * 
 * Preconditions:
 *  process points to a valid PCB
 * Postconditions:
 *  The process's memory is freed for the memory manager, and it is noted as
 *      a FREE_SLOT.
 *  g_LiveProcessCount is decremented.
 *  
 * Thread Safety: None
 */
static void deleteProcess(ProcessInfo_t* process) {
    my_free(process->context.bp);
    process->state = FREE_SLOT;
    g_LiveProcessCount--;
}

/*******************************************************************************
 * Try to wake all the processes whose IOs are done
 * 
 * Preconditions:
 *  None
 * Postconditions:
 *  Processes whose blocking IO has finished are woken up and put in the ready
 *      queue
 *  
 * Thread Safety: None
 */
static void wakeIOs() {
    int idx;
    ProcessInfo_t* curr;
    // Check for IO which is done, and return the process to running
    for (idx = 1; idx < PROC_MAX; idx++) {
        curr = &g_ProcessTable[idx];
        if (curr->state == DOING_IO) {
            if (curr->currentIO->op < 0) {
                curr->state = READY;
                curr->currentIO = NULL;
                Q_Enqueue(g_ReadyQueue, curr);
            }
        }
    }
}
