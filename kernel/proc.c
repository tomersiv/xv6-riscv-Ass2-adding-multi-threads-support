#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

int nexttid = 1;
struct spinlock tid_lock;

extern void forkret(void);

extern void *sigret_start(void);
extern void *sigret_end(void);

static void freeproc(struct proc *p);
void sigcont_handler(void);
void sigstop_handler(void);
void sigkill_handler(void);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc)); //TODO: find the correct offset instead of p - proc
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table at boot time.
void procinit(void)
{
  struct proc *p;
  struct thread *t;

  initlock(&pid_lock, "nextpid");
  initlock(&tid_lock, "nexttid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->thread[0].kstack = KSTACK((int)(p - proc)); //TODO: find the correct offset instead of p - proc

    for (t = p->thread; t < &p->thread[NTHREAD]; t++){
      initlock(&t->lock, "thread");
      t->state = T_UNUSED;
    }
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->thread->parent;
  pop_off();
  return p;
}

struct thread *
mythread(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct thread *t = c->thread;
  pop_off();
  return t;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

int alloctid()
{
  int tid;

  acquire(&tid_lock);
  tid = nexttid;
  nexttid = nexttid + 1;
  release(&tid_lock);

  return tid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;
  p->thread[0].tid = alloctid();
  p->thread[0].state = USED;

  // Allocate a trapframe page.
  if ((p->thread[0].trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->thread[0].context, 0, sizeof(p->thread[0].context));
  p->thread[0].context.ra = (uint64)forkret;
  p->thread[0].context.sp = p->thread[0].kstack + PGSIZE;

  // task 2.1.2 - Updating process creation behavior
  for (int i = 0; i < 32; i++)
  {
    p->sig_handlers[i] = SIG_DFL;
    p->handlers_mask[i] = 0; // task 2.1.4 - blocked signals during handlers execution
  }
  p->pending_sig = 0;
  p->sig_mask = 0;
  p->usertrap_backup = 0;
  // task 2.4 - flag to mark when a user signal is being handled
  p->handling_usersignal = 0;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p) // TODO: implement after kthread_create
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->thread[0].trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// od -t xC initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE; // TODO: check if this field should in proc or in thread

  // prepare for the very first "return" from kernel to user.
  p->thread[0].trapframe->epc = 0;     // user program counter
  p->thread[0].trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  safestrcpy(p->thread[0].name, "Nitzan Ya Gay", sizeof(p->thread[0].name));
  p->cwd = namei("/");

  p->thread[0].state = T_RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
  int i, pid, tid;
  struct proc *np;
  struct thread *nt;
  struct proc *p = myproc();
  struct thread *t = mythread();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  nt = np->thread;

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(nt->trapframe) = *(t->trapframe);

  // Cause fork to return 0 in the child.
  nt->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));
  safestrcpy(nt->name, t->name, sizeof(t->name));

  pid = np->pid;

  // Task 2.1.2 - Updating process creation behavior
  np->sig_mask = p->sig_mask;
  for (i = 0; i < 32; i++)
  {
    np->sig_handlers[i] = p->sig_handlers[i];
    np->handlers_mask[i] = p->handlers_mask[i];
  }

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  nt->state = T_RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *np;
  struct thread *t;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++)
    {
      if (np->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE)
        {
          // for (t = p->thread; t < &p->thread[NTHREAD]; t++)
          // {
          //   if (t->state == T_ZOMBIE)
          //   {

          //   }
          // }

          // Found one.
          pid = np->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0)
          {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed)
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct thread *t;
  struct cpu *c = mycpu();

  c->thread = 0;
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == USED)
      {
        for (t = p->thread; t < &p->thread[NTHREAD]; t++)
        {
          acquire(&t->lock);
          if (t->state == T_RUNNABLE)
          {
            // Switch to chosen process.  It is the process's job
            // to release its lock and then reacquire it
            // before jumping back to us.
            t->state = T_RUNNING;
            c->thread = t;
            swtch(&c->context, &t->context);

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->thread = 0;
          }
          release(&t->lock);
        }
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct thread *t = mythread();

  if (!holding(&t->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (t->state == T_RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&t->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct thread *t = mythread();
  acquire(&t->lock);
  t->state = T_RUNNABLE;
  sched();
  release(&t->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&mythread()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct thread *t = mythread();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  
  acquire(&t->lock); //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  t->chan = chan;
  t->state = T_SLEEPING;

  sched();

  // Tidy up.
  t->chan = 0;

  // Reacquire original lock.
  release(&t->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;
  struct thread *t;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    for (t = p->thread; t < &p->thread[NTHREAD]; t++)
    {
      if (t != mythread())
      {
        acquire(&t->lock);
        if (t->state == T_SLEEPING && t->chan == chan)
        {
          t->state = T_RUNNABLE;
        }
        release(&t->lock);
      }
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid, int signum)
{
  if (signum < 0 || signum > 31)
  {
    return -1;
  }

  struct proc *p;
  struct thread *t;
  
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid && p->sig_handlers[signum] != SIG_IGN)
    {
      p->pending_sig = p->pending_sig | (1 << signum);

      if (signum == SIGKILL)
      {
        int allThreadsSleeping = 1;
        for (t = p->thread; t < &p->thread[NTHREAD]; t++){
          acquire(&t->lock);
          if (t->state != T_SLEEPING) {
            allThreadsSleeping = 0;
            break;
          }
          release(&t->lock);
        }
        
        if (allThreadsSleeping) {
          acquire(&t->lock);
          t--;
          // Wake process from sleep().
          t->state = T_RUNNABLE;
          release(&t->lock);
        }
        else
        {
          release(&t->lock);
        }
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      // [SLEEPING] "sleep ",
      // [RUNNABLE] "runble",
      // [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// Task 2.1.3 - Updating process signal mask
uint sigprocmask(uint sigmask)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  uint oldmask = p->sig_mask;

  if (!(1 << SIGKILL & sigmask) && !(1 << SIGSTOP & sigmask))
  {
    p->sig_mask = sigmask;
  }

  release(&p->lock);
  return oldmask;
}

// Task 2.1.4 - Registering Signal Handlers
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
  struct proc *p = myproc();
  if (signum < 0 || signum > 31 || signum == SIGKILL ||
      signum == SIGSTOP || (act && act->sigmask < 0))
  {
    return -1;
  }
  else
  {
    acquire(&p->lock);
    if (oldact)
    {
      copyout(p->pagetable, (uint64)&oldact->sa_handler, (char *)&p->sig_handlers[signum], sizeof(p->sig_handlers[signum]));
      copyout(p->pagetable, (uint64)&oldact->sigmask, (char *)&p->handlers_mask[signum], sizeof(p->handlers_mask[signum]));
    }
    if (act)
    {
      copyin(p->pagetable, (char *)&p->sig_handlers[signum], (uint64)&act->sa_handler, sizeof(act->sa_handler));
      copyin(p->pagetable, (char *)&p->handlers_mask[signum], (uint64)&act->sigmask, sizeof(act->sigmask));
    }
    release(&p->lock);
    return 0;
  }
}

// Task 2.3.1 - SIGSTOP, SIGKILL and SIGCONT signal handlers
void sigkill_handler()
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

// check if SIGCONT is pending with SIG_DFL as handler, and SIGCONT is not blocked
// returns 1 if the condition is true and 0 if it's false.
int condition1()
{
  struct proc *p = myproc();
  if ((p->pending_sig & (1 << SIGCONT)) && (p->sig_handlers[SIGCONT] == SIG_DFL) &&
      !(p->sig_mask & (1 << SIGCONT)))
  {
    return 1;
  }
  return 0;
}

// check if a signal is pending with SIGCONT as handler, and the signal is not blocked.
// returns the signal number if the condition is true and -1 if it's false.
int condition2()
{
  struct proc *p = myproc();
  int i;
  for (i = 0; i < 32; i++)
  {
    if (((1 << i) & p->pending_sig) && p->sig_handlers[i] == (void *)SIGCONT &&
        !(p->sig_mask & (1 << i)))
    {
      return i;
    }
  }
  return -1;
}

void sigstop_handler()
{
  struct proc *p = myproc();
  int signum = -1;
  acquire(&p->lock);
  while (!condition1() && ((signum = condition2()) == -1))
  {
    yield();
  }

  if (signum == -1)
  {
    p->pending_sig &= ~(1 << SIGCONT);
  }
  else
  {
    p->pending_sig &= ~(1 << signum);
  }
  release(&p->lock);

  return;
}

void sigret(void)
{
  // restore process trapframe and signal mask
  struct proc *p = myproc();
  struct thread *t = mythread();

  acquire(&p->lock);
  copyin(p->pagetable, (char *)t->trapframe, (uint64)p->usertrap_backup, sizeof(struct trapframe));
  p->sig_mask = p->mask_backup;

  // restore stack state to be the state before handeling the signals
  t->trapframe->sp += sizeof(struct trapframe);
  p->handling_usersignal = 0;
  release(&p->lock);
}

// task 2.4
void handle_signal()
{
  struct proc *p = myproc();
  struct thread *t = mythread();

  int i;
  for (i = 0; i < 32; i++)
  {
    acquire(&p->lock);
    if ((p->pending_sig & (1 << i)) && (!p->handling_usersignal) && !((1 << i) & p->sig_mask)) // check if the signal is pending and if we are currently handling a signal
    {
      // handle signals according to signal handler type
      if (p->sig_handlers[i] == (void *)SIG_DFL)
      {
        if (i == SIGSTOP)
        {
          sigstop_handler();
        }
        else if (i == SIGCONT)
        {
          // if SIGCONT arrived before SIGSTOP just clear the appropriate bit in p->pending_signal (at the end of this function)
        }
        else
        {
          sigkill_handler();
        }
      }
      else if (p->sig_handlers[i] == (void *)SIG_IGN)
      {
        // TODO: check if we need to continue; here because we dont want to clear the bit
        // just clear the appropriate bit in p->pending_signal (at the end of this function)
      }
      else if (p->sig_handlers[i] == (void *)SIGKILL)
      {
        sigkill_handler();
      }
      else if (p->sig_handlers[i] == (void *)SIGSTOP)
      {
        sigstop_handler();
      }
      else if (p->sig_handlers[i] == (void *)SIGCONT)
      {
        // if SIGCONT arrived before SIGSTOP just clear the appropriate bit in p->pending_signal (at the end of this function)
      }
      else
      {
        // handle user-space signal handlers
        p->handling_usersignal = 1;
        // TODO: check this at reception hours
        p->sig_mask = p->handlers_mask[i] | (1 << i);

        // before execution of the user-space handler - backup mask
        p->mask_backup = p->sig_mask;

        // backup trapframe
        t->trapframe->sp -= sizeof(struct trapframe);
        p->usertrap_backup = (struct trapframe *)(t->trapframe->sp);
        copyout(p->pagetable, (uint64)p->usertrap_backup, (char *)t->trapframe, sizeof(struct trapframe));

        // "inject" implicit call to sigret system call
        uint64 size = (uint64)&sigret_end - (uint64)&sigret_start;
        t->trapframe->sp -= size;
        copyout(p->pagetable, (uint64)t->trapframe->sp, (char *)&sigret_start, size);
        t->trapframe->a0 = i;                // store signal number
        t->trapframe->ra = t->trapframe->sp; // store sigret system call at the return address

        t->trapframe->epc = (uint64)p->sig_handlers[i]; // program counter points to the user-space handler function
      }
      p->pending_sig &= ~(1 << i);
    }
    release(&p->lock);
  }
}

// Task 3.2 - the first scheduling of a thread created after kthread_create
// will continue from userspace (with the epc set to start_func)
void threadret(void)
{
  release(&mythread()->lock);
  usertrapret();
}

// Task 3.2 - kthread_create syscall
int 
kthread_create(void (*start_func)(), void *stack){
  struct proc *p = myproc();
  struct thread *t;
  
  acquire(&p->lock);
  for (t = p->thread; t < &p->thread[NTHREAD]; t++){
    if (t->state == T_UNUSED) {
      goto found;
    }
    else {
      release(&t->lock);
    }
  }
  return 0;

  found:
  acquire(&wait_lock); // TODO: check if needed!!
  t->parent = p;
  release(&wait_lock);

  initlock(&t->lock, "thread");
  acquire(&t->lock);
  t->state = T_USED;

  release(&p->lock);
  
  t->tid = alloctid();
  t->chan = 0;
  t->killed = 0;
  if((t->kstack = kalloc() == 0))
  {
    freethread(t);
    release(&t->lock);
    return 0;
  }

  t->trapframe = (p->thread[0].trapframe + (int)(t - p->thread) * sizeof(struct trapframe));

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&t->context, 0, sizeof(t->context));
  t->context.ra = (uint64)threadret;
  t->context.sp = t->kstack + PGSIZE;

  t->trapframe->epc = (uint)start_func;
  t->trapframe->sp = stack + MAX_STACK_SIZE;

  acquire(&t->lock);
  t->state = T_RUNNABLE;
  release(&t->lock);
  
  return t->tid;
}

// task 3.2 - kthread_id system call
int kthread_id(void){
  struct thread *t = mythread();
  acquire(&t->lock);
  int tid = t->tid;
  release(&t->lock);
  return tid;
}

void kthread_exit(int *status)
{
  
}