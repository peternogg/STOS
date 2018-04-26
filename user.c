#include "ulib.h"

int main() {
    char* ptr = 0;
    if (prints(ptr) == ERR) {
        prints("Failed to print ptr = 0\n");
    }

    ptr++;
    if (prints(ptr) == ERR) {
        prints("Failed to print ptr = 1\n");
    }
}
