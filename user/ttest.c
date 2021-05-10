#include "../kernel/param.h"
#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"
#include "../kernel/fs.h"
#include "../kernel/fcntl.h"
#include "../kernel/syscall.h"
#include "../kernel/memlayout.h"
#include "../kernel/riscv.h"


#include "../kernel/spinlock.h"  // NEW INCLUDE FOR ASS2
// #include "Csemaphore.h"   // NEW INCLUDE FOR ASS 2
#include "../kernel/proc.h"         // NEW INCLUDE FOR ASS 2, has all the signal definitions and sigaction definition.  Alternatively, copy the relevant things into user.h and include only it, and then no need to include spinlock.h .

int i;

void test_thread(){
    if (i == 1) {
        printf("error/n");
    }
    kthread_exit(0);
}

// void test_thread(){
//     printf("Thread is now running\n");
//     kthread_exit(0);
// }

void thread_test(){
    i = 0;
    int tid;
    int status;
    void* stack = malloc(MAX_STACK_SIZE);
    tid = kthread_create(test_thread, stack);
    kthread_join(tid,&status);

    i++;

    tid = kthread_id();
    free(stack);
    printf("Finished testing threads, main thread id: %d, %d\n", tid,status);
}

// void thread_test(){
//     int tid;
//     int status;
//     void* stack = malloc(MAX_STACK_SIZE);
//     tid = kthread_create(test_thread, stack);
//     printf("Finished creating threads\n");
//     kthread_join(tid,&status);

//     tid = kthread_id();
//     free(stack);
//     printf("Finished testing threads, main thread id: %d, %d\n", tid,status);
// }

int main(int argc, char **argv){
    thread_test();
    exit(0);
}