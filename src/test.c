#define _MULTI_THREADED
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h> // for sleep function


#define NUMTHREADS 3
void sighand(int signo);

void *threadfunc(void *parm)
{
  pthread_t             self = pthread_self();
  int                   rc;

  printf("Thread entered\n");
  rc = sleep(30);
  if (rc != 0) {
    printf("Thread got a signal delivered to it\n");
    return NULL;
  }
  printf("Thread didn't get expected results! rc=%d\n",rc);
  return NULL;
}

int main(int argc, char **argv)
{
  int                     rc;
  int                     i;
  struct sigaction        actions;
  pthread_t               threads[NUMTHREADS];

  printf("Enter Testcase - %s\n", argv[0]);
  
  printf("Set up the alarm handler for the process\n");
  memset(&actions, 0, sizeof(actions));
  sigemptyset(&actions.sa_mask);
  actions.sa_flags = 0;
  actions.sa_handler = sighand;

  rc = sigaction(SIGALRM,&actions,NULL);

  for(i=0; i<NUMTHREADS; ++i) {
    rc = pthread_create(&threads[i], NULL, threadfunc, NULL);
  }

  sleep(1);
  for(i=0; i<NUMTHREADS; ++i) {
    rc = pthread_kill(threads[i], SIGALRM);
  }

  for(i=0; i<NUMTHREADS; ++i) {
    rc = pthread_detach(threads[i]);
  }
  printf("Main completed\n");
  return 0;
}

void sighand(int signo)
{
  pthread_t             self = pthread_self();
  
  printf("Thread in signal handler\n");
  return;
}