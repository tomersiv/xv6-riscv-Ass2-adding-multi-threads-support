#include "kernel/types.h"
#include "user.h"
#include "kernel/fcntl.h"

struct counting_semaphore
{
    int bin_sem1;
    int bin_sem2;
    int value;
};

int csem_alloc(struct counting_semaphore *sem, int initial_value);
void csem_free(struct counting_semaphore *sem);
void csem_down(struct counting_semaphore *sem);
void csem_up(struct counting_semaphore *sem);