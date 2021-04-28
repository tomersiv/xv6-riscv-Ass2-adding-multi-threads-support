struct stat;
struct rtcdate;
struct sigaction;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int, int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
// task 2.1.3 - sigprocmask syscall
int sigprocmask(int);
// task 2.1.4 - sigaction syscall
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
// task 2.1.5 - sigret system call
void sigret(void);
// task 3.2 - kthread_create system call
int kthread_create(void (*start_func)(),void *stack);

// Task 2.1.4 - defining sigaction struct
struct sigaction {
  void (*sa_handler)(int);
  uint sigmask;
};

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
