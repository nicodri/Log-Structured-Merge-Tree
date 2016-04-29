// #include "LSMtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h> // for sleep function
#include <errno.h>

#define VALUE_SIZE 10
#define NUM_THREADS 8
#define FILENAME_SIZE 16

typedef struct arg_read
{
    int thread_id;
    int key;
    // Line to comment
    int Cs_Ne;
    char* filename;

} arg_read;

void *perform_work(void *argument)
{
    arg_read * arg = (arg_read *) argument;

    // Where the work will be done...

    // sleep(1);
    // printf("Hello World! It's me, thread %d with arguments %d, %s!\n", arg->thread_id,
    //        arg->key, arg->filename);
    errno=0;
    int rc = sleep(10);
    if (rc != 0 && errno == EINTR) {
        printf("Thread %d got a signal delivered to it\n",
               arg->thread_id);
        return NULL;
    }

    printf("Thread %d no signal; rc: %d; errno: %d\n", arg->thread_id, rc, errno);
    pthread_exit( (void *) (arg->thread_id));
    // return (void *) (arg->thread_id);
}

static void handler(int signum)
{
    printf("Killing thread %d\n", signum);
    // pthread_exit(NULL);
    return;
}


int main(){
    for (int iteration = 0; iteration < 500; iteration++){
        printf("-----------------------------\n");
        printf("Iteration %d\n", iteration);
        pthread_t threads[NUM_THREADS];
        void *thread_result;
        int result_code, index;

        // First method to Set up the alarm handler for the process
        // struct sigaction        actions;
        // memset(&actions, 0, sizeof(actions));
        // sigemptyset(&actions.sa_mask);
        // actions.sa_flags = 0;
        // actions.sa_handler = handler;
        // int rc = sigaction(SIGALRM,&actions,NULL);

        // Second method to Set up the alarm handler for the process
        signal(SIGALRM, handler);
        printf("Create loop\n");
        for (index = 0; index < NUM_THREADS; ++index) {
            // Wrap up the thread argument into a struct
            arg_read* thread_arg = malloc(sizeof(arg_read)  + (FILENAME_SIZE + 8) * sizeof(char));
            thread_arg->filename = (char *) (thread_arg + sizeof(arg_read));
            thread_arg->thread_id = index;
            thread_arg->key = 15;
            thread_arg->Cs_Ne = 20;
            // sprintf(thread_arg->filename, "test%d.data", index);
            
            // printf("In main: creating thread %d\n", index);
            result_code = pthread_create(&threads[index], NULL,
                                       perform_work, (void *) thread_arg);
            printf("thread %d create %d\n", index, result_code);
            assert(0 == result_code);
        }
        printf("Kill loop\n");
        // wait for each thread to complete
        for (index = 0; index < NUM_THREADS; ++index) {
            // Killing thread
            // printf("In main: killing thread %d\n", index);
            printf("starting kill on thread %d\n", index);
            result_code = pthread_kill(threads[index], SIGALRM);
            printf("thread %d kill %d\n", index, result_code);
            // calling detach to release the thread environment
            // We dont want to have to call pthread_join!!
            // pthread_detach(threads[index]);
        }
        printf("Join loop\n");
        for (index = 0; index < NUM_THREADS; ++index) {
            result_code = pthread_join(threads[index], &thread_result);
            printf("thread %d join %d\n", index, result_code);
        }
    }

}