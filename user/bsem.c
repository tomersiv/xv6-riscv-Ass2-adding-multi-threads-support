#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"


#include "kernel/spinlock.h"  // NEW INCLUDE FOR ASS2
#include "Csemaphore.h"   // NEW INCLUDE FOR ASS 2
#include "kernel/proc.h"         // NEW INCLUDE FOR ASS 2, has all the signal definitions and sigaction definition.  Alternatively, copy the relevant things into user.h and include only it, and then no need to include spinlock.h .



void bsem_test(){
    int pid;
    int bid = bsem_alloc();
    bsem_down(bid);
    printf("1. Parent downing semaphore\n");
    if((pid = fork()) == 0){
        printf("2. Child downing semaphore\n");
        bsem_down(bid);
        printf("4. Child woke up\n");
        exit(0);
    }
    sleep(5);
    printf("3. Let the child wait on the semaphore...\n");
    sleep(10);
    bsem_up(bid);

    bsem_free(bid);
    wait(&pid);

    printf("Finished bsem test, make sure that the order of the prints is alright. Meaning (1...2...3...4)\n");
}
int main(int argc, char **argv)
{
    bsem_test();
    exit(0);
}

