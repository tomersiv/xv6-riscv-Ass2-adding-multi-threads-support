#include "../kernel/types.h"
#include "user.h"
#include "Csemaphore.h"

int csem_alloc(struct counting_semaphore *sem, int initial_value)
{
    //struct counting_semaphore *Csem = (struct counting_semaphore *)malloc(sizeof(struct counting_semaphore));
    int binary_sem1 = bsem_alloc();
    int binary_sem2 = bsem_alloc();
    if (binary_sem1 == -1 || binary_sem2 == -1 || initial_value < 1)
    {
        return -1; // Counting semaphore can only be implemented with two binary semaphores
    }
    sem->bin_sem1 = binary_sem1;
    sem->bin_sem2 = binary_sem2;
    sem->value = initial_value;
    return 0;
}

void csem_free(struct counting_semaphore *sem)
{
    free(sem);
    bsem_free(sem->bin_sem1);
    bsem_free(sem->bin_sem2);
}

void csem_down(struct counting_semaphore *sem)
{
    bsem_down(sem->bin_sem2);
    bsem_down(sem->bin_sem1);
    sem->value--;
    if (sem->value > 0)
    {
        bsem_up(sem->bin_sem2);
    }
    bsem_up(sem->bin_sem1);
}

void csem_up(struct counting_semaphore *sem)
{
    bsem_down(sem->bin_sem1);
    sem->value++;
    if (sem->value == 1)
    {
        bsem_up(sem->bin_sem2);
    }
    bsem_up(sem->bin_sem1);
}