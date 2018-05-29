/*******************************************************************************
 * Author: Peter Higginbotham
 * Do a bunch of work computing by calculating something like 5000!
 */

#include "ulib.h"

#define ITER_COUNT 5000
#define PRINT_INTERVAL 250

void main() {
    int i;
    int acc = 1;
    for (i = 1; i <= ITER_COUNT; i++) {
        acc *= i;
        if (acc == 0)
            acc = i;
        
        if ((i % PRINT_INTERVAL) == 0) {
            printi(acc);
            prints("\n");
        }
    }
}
