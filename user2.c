/************************
 * Author: Phil Howard
 * A test program which can be exec'd
 */
#include "ulib.h"

int main()
{
    int ii;
    sleep(1000000);
    for (ii=0; ii<20; ii++)
    {
        printi(ii);
        prints(" two\n");
    }

    prints("Finished two!\n");

    return 0;
}
