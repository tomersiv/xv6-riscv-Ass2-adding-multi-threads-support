#include "kernel/types.h"
#include "kernel/fs.h"

#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"

#define NUM_OF_CHILDREN 20
#define SIG_FLAG1 10
#define SIG_FLAG2 20

int flag1 = 0;
int flag2 = 0;
int flag3 = 0;
int count = 0;

void setFlag1(int);
void setFlag2(int);
void setFlag3(int);
void incCount(int);
void setMask(int);
void itoa(int, char *);
void usrKillTest(void);
void reverse(char[]);

// void killTest(){
//   printf("starting kill test\n");
//   int children[NUM_OF_CHILDREN];
//   for(int i=0;i<NUM_OF_CHILDREN;++i){
//     children[i] = fork();
//     if(children[i]==0){while(1);exit(1);}
//   }
//   for(int i=0;i<NUM_OF_CHILDREN;++i){
//     kill(children[i],SIGKILL);
//   }
//   for(int i=0;i<NUM_OF_CHILDREN;++i){wait(1);}
//   printf(2,"All children killed!\n");
// }

void maskChangeTest()
{
    printf("Starting mask change test\n");
    uint origMask = sigprocmask((1 << 2) | (1 << 3));
    if (origMask != 0)
    {
        printf("Original mask wasn't 0. Test failed\n");
        return;
    }
    uint secMask = sigprocmask((1 << 2) | (1 << 3) | (1 << 4));
    if (secMask != 12)
    {
        printf("Mask wasn't changed. Test failed\n");
        return;
    }
    secMask = sigprocmask((1 << 2) | (1 << 3) | (1 << 4));
    if (secMask != 28)
    {
        printf("Mask wasn't changed. Test failed\n");
        return;
    }
    if (fork() == 0)
    {
        secMask = sigprocmask((1 << 2) | (1 << 3) | (1 << 4));
        if (secMask != 28)
        {
            printf("Child didn't inherit father's signal mask. Test failed\n");
        }
        // else
        // {
        //     char *argv[] = {"signalTest", "sigmask", 0};
        //     if (exec(argv[0], argv) < 0)
        //     {
        //         printf("couldn't exec, test failed\n");
        //         exit(1);
        //     }
        // }
    }
    wait((int *)0);
    sigprocmask(0);
}

void handlerChange()
{
    printf("Starting handler change test\n");
    struct sigaction sig1;
    struct sigaction sig2;
    sig1.sa_handler = &setFlag1;
    sig1.sigmask = 0;
    sig2.sa_handler = 0;
    sig2.sigmask = 0;

    sigaction(2, &sig1, &sig2);
    if (sig2.sa_handler != (void *)0)
    {
        printf("Original wasn't 0. failed\n");
        return;
    }
    sig1.sa_handler = &setFlag2;
    sigaction(2, &sig1, &sig2);
    printf("%p\n", sig2.sa_handler);

    if (sig2.sa_handler != &setFlag1)
    {
        printf("Handler wasn't changed to custom handler, test failed\n");
        return;
    }

    sig1.sa_handler = (void *)1;
    sig1.sigmask = 5;
    sigaction(2, &sig1, &sig2);

    if (fork() == 0)
    {
        if (sig2.sa_handler != &setFlag2)
        {
            printf("Signal handlers changed after fork. test failed\n");
            return;
        }
    }

    wait((int *)0);
    return;
}

void multipleChildrenTest()
{
    count = 0;
    int numOfSigs = 32;
    struct sigaction sig1;
    struct sigaction sig2;
   
    printf("%p\n",&incCount);
    sig1.sa_handler = &incCount;
    sig1.sigmask = 0;
    for (int i = 0; i < numOfSigs; ++i)
    {
        sigaction(i, &sig1, &sig2);
    }
    sigaction(31, &sig1, &sig2);
    sigaction(31, &sig1, &sig2);
    printf("%p\n",sig2.sa_handler);
    int pid = getpid();
    for (int i = 0; i < numOfSigs; ++i)
    {
        int child = fork();
        if (child == 0)
        {
            int signum = i;
            if (signum!=9 && signum!=17 && signum!=19){
            kill(pid, signum);
            printf("exited %d\n",i);
            
            }
            exit(0);
        }
    }
   for (int i = 0; i < numOfSigs; ++i)
    {
        wait((int *)0);
        
    }
    while (1)
    {
        printf("count is: %d\n", count);
        if (count == numOfSigs -3 )
        {
            printf("All signals received!\n");
            break;
        }
    }
     
    sig1.sa_handler = 0;
    for (int i = 0; i < numOfSigs; ++i)
    {
        sigaction(i, &sig1, 0);
    }
    printf("multiple children test passed\n");
}

// void maskChangeSignalTest(){
//   printf(2,"Starting mask change signal test\n");
//   signal(2,&setMask);
//   signal(3,&setFlag3);
//   signal(8,&setFlag2);
//   int pid=getpid();
//   int child=fork();
//   if(child==0){
//     int count=0;
//     while(1){
//       if(flag1){
//         count++;
//         flag1=0;
//         printf(2,"sending signal to father\n");
//         kill(pid,8);
//       }
//       if(count==2){
//         exit();
//       }
//       if(flag3){
//         kill(pid,8);
//         exit();
//       }
//     }
//   }
//   printf(2,"sending signal to child\n");
//   kill(child,2);
//   int count=0;
//   while(1){
//     if(flag2){
//       flag2=0;
//       count++;
//       printf(2,"received signal from child\n");
//       printf(2,"trying to send signal again. Then sleeping for 500\n");
//       kill(child,2);
//       sleep(500);
//       if(flag2){
//         printf(2,"Test failed. Shouldn't have received signal back.\n");
//         kill(child,SIGKILL);
//         break;
//       }
//       printf(2,"did not receive signal from child, sending a different one.\n");
//       kill(child,3);
//       printf(2,"busy waiting for answer\n");
//       while(!flag2);
//       printf(2,"received signal from child.\n");
//       wait();
//       printf(2,"Mask change signal test passed\n");
//       break;
//     }
//   }
// }

// void communicationTest(){
//   printf(2,"starting communication test\n");
//   signal(SIG_FLAG1,&setFlag1);
//   signal(SIG_FLAG2,&setFlag2);
//   int father=getpid();
//   int child=fork();
//   if(!child){
//     while(1){
//       if(flag1){
//         flag1=0;
//         printf(2,"received 10 signal. sending 20 signal to father.\n");
//         kill(father,SIG_FLAG2);
//         printf(2,"exiting\n");
//         exit();
//       }
//     }
//   }
//   printf(2,"sending 10 signal to child, waiting for response\n");
//   kill(child,SIG_FLAG1);
//   while(1){
//     if(flag2){
//       printf(2,"received signal from child\n");
//       flag2=0;
//       wait();
//       break;
//     }
//   }
//   printf(2,"communication test passed\n");
// }

// void multipleSignalsTest(){
//   count=0;
//   signal(2,&setFlag1);
//   for(int i=3;i<9;++i){signal(i,&incCount);}
//   int child;
//   if((child=fork())==0){
//     while(1){
//       if(flag1){
//         printf(2,"I'm printing number %d\n",count);
//       }
//       if(count==6){
//         printf(2,"Count is %d\n",count);
//         count=0;
//         flag1=0;
//         exit();
//       }
//     }
//   }
//   printf(2,"Sending signal to child\n");
//   kill(child,2);
//   printf(2,"Sending multiple signals to child\n");
//   for(int i=3;i<9;++i){kill(child,i);}
//   for(int i=0;i<6;++i){wait();}
//   printf(2,"Multiple signals test passed\n");
// }

// void stopContTest(){
//   int child;
//   signal(4,&setFlag1);
//   if((child=fork())==0){
//     while(1){
//       printf(2,"Running!!!\n");
//       if(flag1){
//         printf(2,"Received signal!\n");
//         flag1=0;
//       }
//     }
//   }
//   sleep(5);
//   printf(2,"Stopping child!\n");
//   kill(child,SIGSTOP);
//   while(!isStopped(child));
//   printf(2,"Child stopped! Sending child a signal, sleep for 10 and then continuing the child.\n");
//   kill(child,4);
//   sleep(10);
//   kill(child,SIGCONT);
//   while(isStopped(child));
//   kill(child,SIGKILL);
//   wait();
//   printf(2,"child killed!\n");
//   printf(2,"stop cont test passed\n");
// }

// void usrKillTest() {
//   int i, j, pid, maxNumOfDigits = 5, numOfSigs = 15;
//   int pids[numOfSigs];
//   char argStrings[numOfSigs*2][maxNumOfDigits];
//   char* argv[numOfSigs*2+2];
//   for(i=0;i<numOfSigs;++i){argv[2*i+1]=argStrings[i];}
//   for (i=0; i<numOfSigs; i++) {
//     if ((pid = fork()) != 0) {  //father
//       pids[i] = pid;
//     }
//     else break; //son
//   }
//   if (!pid) {
//     while(1);
//   }

//   for (i=0; i<numOfSigs; i++) {itoa(pids[i], argv[2*i+1]);}
//   argv[0] = "kill";
//   j=0;
//   for (i=0, j=0; i<numOfSigs; i++, j=j+2) {
//     argv[j+2] = "9";
//   }
//   argv[j+1] = 0;
//   j=0;
//   if(!fork()){
//     if(exec(argv[0],argv)<0){
//         printf(2,"couldn't exec kill, test failed\n");
//         exit();
//     }
//   }
//   wait();
//   for(int i=0;i<numOfSigs;i++){
//     printf(2,"another one bites the dust\n");
//     wait();
//   }
//   printf(2,"User Kill Test Passed\n");
// }

// void sendSignalUsingKillProgram(){
//   printf(2,"starting signal test using kill prog\n");
//   signal(SIG_FLAG1,&setFlag1);
//   signal(SIG_FLAG2,&setFlag2);
//   int pid=getpid();

//   char pidStr[20];
//   char signumStr[3];
//   char* argv[] = {"kill",pidStr,signumStr,0};

//   itoa(pid,pidStr);
//   int child = fork();
//   if(child==0){
//     while(1){
//       if(flag1){
//         flag1=0;
//         itoa(SIG_FLAG2,signumStr);
//         if(fork()==0){
//           if(exec(argv[0],argv)<0){
//             printf(2,"couldn't exec. test failed\n");
//             exit();
//           }
//         }
//         exit();
//       }
//     }
//   }
//   itoa(child,pidStr);
//   itoa(SIG_FLAG1,signumStr);
//   if(fork()==0){
//     if(exec(argv[0],argv)<0){
//       printf(2,"couldn't exec. test failed\n");
//       exit();
//     }
//   }
//   while(1){
//     if(flag2){
//       flag2=0;
//       wait();
//       break;
//     }
//   }
//   printf(2,"signal test using kill prog passed\n");
// }

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "sigmask") == 0)
        {
            uint secMask = sigprocmask(0);
            if (secMask != 28)
            {
                printf("Mask was changed after exec, test failed.\n");
                exit(1);
            }
            printf("Mask change test passed\n");
            exit(1);
        }
    }
    //     if(strcmp(argv[1],"sighandlers")==0){
    //       if(signal(3,(void*)SIG_IGN)!=(void*)SIG_IGN){
    //         printf(2,"SIG_IGN wasn't kept after exec. test failed\n");
    //         exit();
    //       }
    //       if(signal(4,&setFlag1)==&setFlag1){
    //         printf(2,"Custom signal handler wasn't removed after exec. test failed\n");
    //         exit();
    //       }
    //       printf(2,"Handler change test passed\n");
    //       exit();
    //     }
    //     printf(2,"Unknown argument\n");
    //     exit();
    //   }
    //   usrKillTest();
    //   sendSignalUsingKillProgram();
    multipleChildrenTest();
    //   killTest();
    //   stopContTest();
    //   communicationTest();

    //maskChangeTest();
    //handlerChange();
    //   maskChangeSignalTest();
    //   multipleSignalsTest();
    exit(0);
}

void setMask(int signum)
{
    flag1 = 1;
    sigprocmask(1 << signum);
}

void setFlag1(int signum)
{
    flag1 = 1;
    return;
}

void setFlag2(int signum)
{
    flag2 = 1;
    return;
}

void setFlag3(int signum)
{
    flag3 = 1;
    return;
}

void incCount(int signum)
{
    printf("in incount\n");
    count++;
}

// void itoa(int num,char* str){
//
// }

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do
    {                          /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void reverse(char s[])
{
    int i, j;
    char c;
    char *curr = s;
    int stringLength = 0;
    while (*(curr++) != 0)
        stringLength++;

    for (i = 0, j = stringLength - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}