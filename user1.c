#include "ulib.h"

int main()
{
    int ii;

    exec("user2.slb");

    for (ii=0; ii<50; ii++)
    {
        printi(ii);
        prints(" one\n");
    }

    exec("user3.slb");

    for (ii=50; ii<100; ii++)
    {
        printi(ii);
        prints(" one\n");
    }

    exit();

    return 0;
}
