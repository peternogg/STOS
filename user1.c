/************************
 * Author: Phil Howard
 * Modified by: Peter Higginbotham
 * A test program which launches a pair of programs to test execs
 */
#include "ulib.h"

int main()
{
    int ii;
    int u1;
    int u2;
    //prints("Hello");
    
    //sleep(10000);
    u1 = exec("user2.slb");

    for (ii=0; ii<50; ii++)
    {
        //sleep(10000);
        printi(ii);
        prints(" one\n");
    }
    
    wait(u1);
    u2 = exec("user3.slb");


    for (ii=50; ii<100; ii++)
    {
        printi(ii);
        prints(" one\n");
    }

    prints("Finished one!\n");
    wait(u2);

    exit();

    return 0;
}
