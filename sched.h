#pragma once

#include <string.h>
#include <syscodes.h>
#include <machine_def.h>
#include <timer.h>

#include "mymalloc.h"
#include "kerncommon.h"
#include "syscalls.h"
#include "queue.h"

#define NO_SLOTS_AVAILABLE -1
#define OUT_OF_MEMORY -2

// Process states
#define RUNNING     0x01
#define READY       0x02
#define LOADING     0x04
#define FREE_SLOT   0x08
#define SLEEPING    0x10
#define DOING_IO    0x20
#define WAITING     0x40
#define ZOMBIE      0x80

#define PROG_NAME_LENGTH 30

typedef struct {
    int sp;
    int flag;
    int bp;
    int lp;
    int ip;
    int fp;
} ProcessorState_t;

typedef struct {
    // The process's state
    int state; 
    // The process's context (register set)
    ProcessorState_t context;
    // System time to sleep until
    int wakeAt;
    // The most recent IO request the process issued
    InpArg* currentIO;
    // The PID of the process which started this one
    int parent;
    // The PID of whatever process is waiting on this process
    int waitedOnBy;
    // The exectuable's name, null terminated
    char pname[PROG_NAME_LENGTH];
} ProcessInfo_t;

void sched_init();
int  sched_userExec(SyscallArg_t* argument, ProcessorState_t* context);
int  sched_exec(char* filename);
void sched_next(ProcessorState_t* context);
void sched_exitCurrent(ProcessorState_t* context);
void sched_sleepCurrent(ProcessorState_t* context, int wakeAt);
int  sched_waitOn(ProcessorState_t* context, int waitPID);
void sched_BeginIO(ProcessorState_t* context, InpArg* ioInfo);
int  sched_getPID();
int  sched_getPPID();
