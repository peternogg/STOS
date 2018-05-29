/************************
 * Author: Peter Higginbotham 
 * A simple shell for StacklOS which demonstrates some of its features
 */
#include <string.h>
#include "ulib.h"

static void handleCommand(char* cmd);
static void printHelp();
static void listPrograms();

void main() {
    int stayAlive = 1;
    int childPID = 0;
    int pid = getpid();
    char buffer[256];

    prints("== Shell (");
    printi(pid);
    prints(") ==\n");
    while(stayAlive) {
        memset(buffer, 0, sizeof(buffer));
        prints("Shell> ");
        gets(buffer);

        // Delete the last newline in buffer
        buffer[strlen(buffer) - 1] = 0;
        handleCommand(buffer);
    }
}

static void handleCommand(char* cmd) {
    int childPID;
    if (strcmp(cmd, "exit") == 0) {
        if (getppid() == 0)
            halt();
        else
            exit();
    } else if (strcmp(cmd, "halt") == 0)
        halt();
    else if (strcmp(cmd, "help") == 0)
        printHelp();
    else if (strcmp(cmd, "ls") == 0)
        listPrograms();
    else {
        childPID = exec(cmd);
        if (childPID != ERR) {
            if (wait(childPID) != OK)
                prints("Error: wait failed\n");
        } else {
            prints("Error: '");
            prints(cmd);
            prints("' is not an executable program.\n");
        }
    }
}

static void printHelp() {
    prints("This is a simple shell. Please don't be too mean to it.\n");
    prints("Supported commands:\n");
    prints("    help: Shows this help information.\n");
    prints("    ls: List the available programs this shell can run.\n");
    prints("    exit: Exits this shell. If the shell's parent is 0, then stop the OS.\n");
    prints("    halt: Halt the stackl machine immediately.\n");
}

static void listPrograms() {
    prints("== Known Programs ==\n");
    prints("    shell: This program. Launches other programs.\n");
    prints("    user: Creates a bunch of output and tests some user library calls\n");
    prints("    user1: Prints some numbers and starts user2 and user3\n");
    prints("    user2: Prints the numbers 0 to 19\n");
    prints("    user3: Prints the numbers 0 to 99\n");
    prints("    nothing: Does nothing. Starts and then exits. Simple test program.\n");
    prints("    factorial: Calculates a large number, like 5000!, and prints it every 250 iterations.\n");
    prints("    2factorial: Launches 2 factorials simultaneously to verify that two takes almost the same time as one.\n");
    prints("Other programs placed in this directory and which are executable by the stackl machine should also work\n");
    prints("So long as only one at a time asks for input.\n");
}
