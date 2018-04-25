#include "ulib.h"

int main() {
    char buffer[30];
    int age;
    if (prints("Hello! What's your name? ") == -1) {
        prints("Uh oh! Error in printing.\n");
    }
    
    gets(buffer);

    prints("Hello ");
    prints(buffer);
    prints("!\nAnd how old are you?");
    age = geti();
    prints("Cool! You're ");
    printi(age);
    prints(".\n");
    halt();    
}
