#include "kernel/types.h"
#include "user.h"

int main(int argc, char **argv)
{
    struct sigaction oldact;

    for (int i = 0; i < 32; i++)
    {
        sigaction(i, 0, &oldact);
        if (i == 5 && oldact.sa_handler != (void *)SIG_IGN)
        {
            fprintf(1, "exec test: failed to copy SIG_IGN handler\n");
            exit(1);
        }
        else if (i != 5 && oldact.sa_handler != (void *)SIG_DFL)
        {
            fprintf(1, "exec test: failed to set handlers to SIG_DFL\n");
            exit(1);
        }
    }
    fprintf(1, "exec test: success\n");
    exit(0);
}