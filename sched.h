#pragma once

#define NO_SLOTS_AVAILABLE 1
#define OUT_OF_MEMORY 2

// Process states
#define RUNNING 0x01
#define READY 0x02
#define LOADING 0x04
#define FREE_SLOT 0x08

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
    char state; 
    // The process's context (register set)
    ProcessorState_t context;
    // The exectuable's name, null terminated
    char pname[PROG_NAME_LENGTH];
} ProcessInfo_t;

void sched_init();
int sched_exec(char* filename);
void sched_next(ProcessorState_t* context);
void sched_exitCurrent(ProcessorState_t* context);
