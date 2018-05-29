/************************
 * Author: Phil Howard
 * A test program which can be exec'd
 */
#include "ulib.h"

int main()
{
    int ii;
    for (ii=0; ii<100; ii++)
    {
        printi(ii);
        prints(" three\n");
    }

    prints("Finished three!\n");

    exit();

    return 0;
}
