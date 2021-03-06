#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct thread *t = mythread();
  switch (n) {
  case 0:
    return t->trapframe->a0;
  case 1:
    return t->trapframe->a1;
  case 2:
    return t->trapframe->a2;
  case 3:
    return t->trapframe->a3;
  case 4:
    return t->trapframe->a4;
  case 5:
    return t->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)
    return -1;
  return fetchstr(addr, buf, max);
}

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);


// Task 2.1.3 - sigprocmask system call
extern uint64 sys_sigprocmask(void);

// Task 2.1.4 - sigaction system call
extern uint64 sys_sigaction(void);

// task 2.1.5 - sigret system call
extern uint64 sys_sigret(void);

// task 3.2 - kthread_create system call
extern uint64 sys_kthread_create(void);

// task 3.2 - kthread_id system call
extern uint64 sys_kthread_id(void);

// task 3.2 - kthread_join system call
extern uint64 sys_kthread_join(void);

// task 3.2 - kthread_exit system call
extern uint64 sys_kthread_exit(void);

// task 4.1 - binary semaphore system calls
extern uint64 sys_bsem_alloc(void);
extern uint64 sys_bsem_free(void);
extern uint64 sys_bsem_down(void);
extern uint64 sys_bsem_up(void);

static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
// Task 2.1.3 - sigprocmask system call
[SYS_sigprocmask]  sys_sigprocmask,
// Task 2.1.4 - sigaction system call
[SYS_sigaction]   sys_sigaction,
// task 2.1.5 - sigret system call
[SYS_sigret]   sys_sigret,
// task 3.2 - kthread_create system call
[SYS_kthread_create]  sys_kthread_create,
// task 3.2 - kthread_id system call
[SYS_kthread_id]  sys_kthread_id,
// task 3.2 - kthread_join system call
[SYS_kthread_join]  sys_kthread_join,
// task 3.2 - kthread_exit system call
[SYS_kthread_exit]  sys_kthread_exit,
// task 4.1 - binary semaphore system calls
[SYS_bsem_alloc] sys_bsem_alloc,
[SYS_bsem_free] sys_bsem_free,
[SYS_bsem_down] sys_bsem_down,
[SYS_bsem_up] sys_bsem_up
};

void
syscall(void)
{
  int num;
  struct thread *t = mythread();

  num = t->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    t->trapframe->a0 = syscalls[num]();
  } else {
    printf("%d %s: unknown sys call %d\n",
            t->tid, t->name, num);
    t->trapframe->a0 = -1;
  }
}
