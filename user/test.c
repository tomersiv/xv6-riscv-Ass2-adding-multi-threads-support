#include "kernel/types.h"
#include "user.h"

int main(int argc, char **argv){
    
    // default handlers test
    struct sigaction oldact;    
    for (int i = 0; i < 32; i++){
        sigaction(i, 0, &oldact);
        if (oldact.sa_handler != SIG_DFL) {
            fprintf(1, "default handlers intiliaztion: failed\n");
        }
    }
    fprintf(1, "default handlers intiliaztion: succeses\n");

    struct sigaction act = { (void *)SIG_IGN, 1125};
    sigaction(5, &act, 0);
    sigprocmask(300);
    int i = sigprocmask(300);

    // sigprocmask test
    if (i == 300) {
        fprintf(1, "sigprocmask test: success\n");
    }
    
    int cpid; 
    if ((cpid = fork()) == 0) {
        sigaction(5, 0, &oldact);
        i = sigprocmask(0);
        if (oldact.sa_handler == (void *)SIG_IGN && i == 300) {
            fprintf(1, "fork handlers and sigmask inherit: success\n");
        }
        else {
            fprintf(1, "fork handlers and sigmask inherit: failed\n");
        }
    }
    else {
        wait(0);
    }

    // exec sighandlers and sigmask initializtion
    for (i = 0; i < 32 && i != 5 && i != SIGSTOP && i != SIGKILL; i++){
        act.sa_handler = (void *)SIGSTOP;
        sigaction(i, &act, 0);
    }

    if (cpid == 0) {
        char *args[]={"test2",0};
        exec(args[0],args);
    }
    else {
        wait(0);
    }

    exit(0);
}
