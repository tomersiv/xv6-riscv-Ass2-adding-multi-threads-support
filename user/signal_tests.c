#include "kernel/types.h"
#include "user.h"

int wait_sig = 0;

void test_handler(int signum){
    wait_sig = 1;
    printf("Received sigtest\n");
}
void test_handler2(int signum){
    wait_sig = 1;
    printf("Received sigtest\n");
}


void signal_test(){
    int pid;
    int testsig;
    testsig=15;
    struct sigaction act = {test_handler2, (uint)(1 << 29)};
    struct sigaction old;

    sigprocmask(0);
    sigaction(testsig, &act, &old);
    if((pid = fork()) == 0){
        while(!wait_sig)
            sleep(1);
        exit(0);
    }
    kill(pid, testsig);
    wait(&pid);
    printf("Finished testing signals\n");
}

void sigkill_test(){
    int pid;
    int testsig;
    testsig=SIGKILL;

    if((pid = fork()) == 0){
        while(!wait_sig)
        ;
        exit(0);
    }
    kill(pid, testsig);
    wait(&pid);
    printf("Finished testing sigkill\n");
}


// child process should print a message in an endless loop
// father will send SIGSTOP to childm then after 10 ticks
// will send sigcont. the child should continue the endless
// loop. ** THE EXPECTED RESULT IS AN ENDLESS LOOP **
void sigstop_test(){
    int pid;
    int testsig;
    testsig=SIGSTOP;
    struct sigaction oldact1;
    struct sigaction act1 = { (void *)SIG_IGN, 1125};
    sigaction(SIGCONT, &act1, &oldact1);

    if ((pid = fork()) == 0) {
        while(!wait_sig) {
            printf("child is waiting for sigstop\n");
        }
        printf("child process resumed and now exiting\n");
        exit(0);
    }
    sleep(5);
    printf("sending sigstop to child\n");
    kill(pid, testsig);
    sleep(15);
    printf("sending sigcont to child\n");
    kill(pid, SIGCONT);
    exit(0);
}

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
        
    wait(0);

    signal_test();

    wait_sig = 0;
    sigkill_test();

    wait_sig = 0;
    sigstop_test();

    exit(0);
}