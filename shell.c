#include <string.h>
#include "ulib.h"

void main() {
    int stayAlive = 1;
    int childPID = 0;
    char buffer[256];

    while(stayAlive) {
        memset(buffer, 0, sizeof(buffer));
        prints("Shell> ");
        gets(buffer);

        // Delete the last newline in buffer
        buffer[strlen(buffer) - 1] = 0;

        if (strcmp(buffer, "exit") == 0)
            exit();
        else {
            childPID = exec(buffer);
            //printi(childPID);
            if (childPID != ERR) {
                //prints("Started ");
                //printi(childPID);
                //prints("\n");
                if (wait(childPID) != OK) {
                    prints("Error: wait failed\n");
                } else {
                    //prints("==> Finished process.");
                }
            } else {
                prints("Error: '");
                prints(buffer);
                prints("' is not an executable program.\n");
            }
        }
    }
}
