#include "ulib.h"

int main()
{
    int ii;
    prints("Hello");
    
    exec("user2.slb");
    exec("user3.slb");

    for (ii=0; ii<50; ii++)
    {
        printi(ii);
        prints(" one\n");
    }


    for (ii=50; ii<100; ii++)
    {
        printi(ii);
        prints(" one\n");
    }

    prints("Finished one!\n");

    exit();

    return 0;
}
