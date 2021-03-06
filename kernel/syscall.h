// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21

// Task 2.1.3 - sigprocmask system call
#define SYS_sigprocmask 22
// task 2.1.4 - sigaction system call
#define SYS_sigaction 23
// task 2.1.5 - sigret system call
#define SYS_sigret 24
// task 3.2 - kthread_create system call
#define SYS_kthread_create 25
// task 3.2 - kthread_id system call
#define SYS_kthread_id 26
// task 3.2 - kthread_join system call
#define SYS_kthread_join 27
// task 3.2 - kthread_exit system call
#define SYS_kthread_exit 28
// task 4.1 - binary semaphore system calls
#define SYS_bsem_alloc 29
#define SYS_bsem_free 30
#define SYS_bsem_down 31
#define SYS_bsem_up 32

