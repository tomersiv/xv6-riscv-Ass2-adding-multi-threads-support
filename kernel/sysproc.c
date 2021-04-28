#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid, signum;

  if(argint(0, &pid) < 0 || argint(1, &signum) < 0)
    return -1;
  return kill(pid, signum);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// Task 2.1.3 - sigprocmask system call
uint64 
sys_sigprocmask(void)
{
  int mask;
  if(argint(0, &mask) < 0)
    return -1;
  return sigprocmask(mask);  
}

// Task 2.1.4 - sigaction system call
uint64
sys_sigaction(void){
  int signum;
  const struct sigaction *act;
  struct sigaction *oldact;

  if(argint(0, &signum) < 0 || argaddr(1, (uint64 *)&act) < 0 || argaddr(2, (uint64 *)&oldact) < 0)
    return -1;

  return sigaction(signum, act, oldact);
}

// task 2.1.5 - sigret system call
uint64
sys_sigret(void){
  sigret();
  return 0;
}

// 3.2 - kthread_create system call
uint64
sys_kthread_create(void){
  void (*start_func)();
  void *stack;
  if (argaddr(0, (uint64 *)&start_func) < 0 || argaddr(1, (uint64 *)&stack) < 0){
    return -1;
  }
  return kthread_create(start_func, stack);
}