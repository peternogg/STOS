/************************
 * Peter Higginbotham 
 * This file contains the user program to test both the kernel and ulib library.
 */

#include "ulib.h"

char* globalString;
char globalBuffer[256];

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void displayMessage(char* msg) {
    prints("==> ");
    prints(msg);
    prints("\n");
}

/******************************************************************************/
// Thread Safety: None
void checkPrintingFromGlobalSucceeds() {
    int err = prints(globalString);
    if (err == ERR)
        displayMessage("Printing from global FAILED");
    else
        displayMessage("Printing from global succeeded");
}

/******************************************************************************/
// Thread Safety: None
void checkReadingIntoAGlobalBufferSucceeds() {
    int err;
    prints("Enter some text: ");
    err = gets(globalBuffer);

    if (err == ERR)
        displayMessage("Getting into global buffer FAILED");
    else {
        displayMessage("Getting into global buffer succeeded");
        prints("You entered: ");
        prints(globalBuffer);
    }
}

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void checkReadingIntoAPointerTooCloseToLPFails() {
    char* ptr;
    int bp;
    bp = asm2("PUSHREG", BP_REG);
    ptr = asm2("PUSHREG", LP_REG);

    // Put ptr 40 bytes from LP and hope that there's not a null there
    ptr -= bp;
    ptr -= 40;

    if (prints(ptr) == ERR)
        displayMessage("Printing from too close to LP failed (Good)");
    else
        displayMessage("Printing from too close to LP SUCCEEDED (Bad)");
}

/******************************************************************************/
// Thread Safety: Safe as long as reading and printing are threadsafe
void checkReadingIntWorks() {
    prints("Enter an integer: ");
    int i = geti();

    prints("==> Got the value ");
    printi(i);
    prints(".\n");
}

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void checkPrintingIntegersWorks() {
    prints("This should print 123\n==> ");
    printi(123);
    prints("\nThis should print -1\n==> ");
    printi(-1);
    prints("\nThis should print INT_MAX (2147483647)\n==> ");
    printi(0x7FFFFFFF);
    prints("\nThis should print INT_MIN (-2147483648), but it prints '-'\n==> ");
    printi(0x7FFFFFFF + 1);
    prints("\n");
}

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void checkPrintingPastLPFails() {
    char* ptr;
    int bp;
    ptr = asm2("PUSHREG", LP_REG);
    bp = asm2("PUSHREG", BP_REG);

    ptr -= bp;
    ptr += 10;

    if (prints(ptr) == ERR)
        displayMessage("Printing past the LP failed (Good)");
    else
        displayMessage("Printing past the LP SUCCEEDED (BAD)");
}

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void checkPrintingNULLFails() {
    if (prints(NULL) == ERR)
        displayMessage("Printing NULL failed (Good)");
    else
        displayMessage("Printing NULL SUCCEEDED (BAD)");
}

/******************************************************************************/
// Thread Safety: Safe as long as printing is threadsafe
void checkPrintingFromRightUnderStringLimitSucceeds() {
    int bp;
    char* ptr;

    bp = asm2("PUSHREG", BP_REG);
    ptr = asm2("PUSHREG", LP_REG);
    ptr -= bp;
    ptr -= 4; // Back up one word to avoid machine check

    // Put a null character right before LP
    *ptr = (char)0; 

    if (prints(ptr) == OK)
        displayMessage("Printing just before the LP limit succeeded");
    else
        displayMessage("Printing just before the LP limit FAILED");
}

void checkPrintingAndBeyondLPFails() {
    int lp;
    int bp;
    bp = asm2("PUSHREG", BP_REG);
    lp = asm2("PUSHREG", LP_REG);

    char* target = (lp - bp);
    if (prints(target) == ERR) 
        displayMessage("Printing at LP failed (Good)");
    else
        displayMessage("Printing at LP SUCCEEDED (BAD)");

    target += 100;
    if (prints(target) == ERR) 
        displayMessage("Printing past LP failed (Good)");
    else
        displayMessage("Printing past LP SUCCEEDED (BAD)");
}

/******************************************************************************/
// Thread Safety: None
int main() {
    globalString = "This is the global string!\n";

    checkPrintingFromGlobalSucceeds();
    checkReadingIntoAGlobalBufferSucceeds();
    checkReadingIntoAPointerTooCloseToLPFails();
    checkReadingIntWorks();
    checkPrintingIntegersWorks();
    checkPrintingPastLPFails();
    checkPrintingNULLFails();
    checkPrintingFromRightUnderStringLimitSucceeds();
    checkPrintingAndBeyondLPFails();
}
