/*******************************************************************************
 * Author: Peter Higginbotham
 * Launch two instances of factorial to do some timing
 */

#include "ulib.h"

void main() {
    int c1;
    int c2;
    c1 = exec("factorial");
    c2 = exec("factorial");

    wait(c1);
    wait(c2);
}
