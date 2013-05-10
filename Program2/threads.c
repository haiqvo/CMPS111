//
//  threads.c
//  
//
//  Created by Scott Brandt on 5/6/13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

static ucontext_t ctx[3];

static void test_thread(void);
void thread_exit(int);

struct thread_node
{
    int id;
    struct thread_node* next;
    struct thread_node* prev;
    ucontext_t ctx;
    bool active;
};
typedef struct thread_node* thread_ref;

thread_ref thread = NULL;
static int threads = 0;
static int thread_count = 0;
thread_ref main_thread = NULL;

thread_ref make_node(void)
{
    thread_ref new = malloc(sizeof(struct thread_node));
    new->id = threads;
    new->active = true;
    //new->ctx;
    if(thread == NULL)
    {
        new->next = new;
        new->prev = new;
        thread = new;
        main_thread = thread;
    }
    else 
    {
        new->next = thread->next;
        new->next->prev = new;
        new->prev = thread;
        thread->next = new;
    }
    return new;
}
// This is the main thread
// In a real program, it should probably start all of the threads and then wait for them to finish
// without doing any "real" work

int inc_threads(void)
{
    thread_count++;
    return ++threads;
}

int main(void) {
    srand(time(NULL));
    printf("Main starting\n");
    make_node();
    
    printf("Main calling thread_create\n");

    // Start one other thread
    thread_create(&test_thread);
    
    printf("Main returned from thread_create\n");

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        printf("Main calling thread_yield\n");
        
        thread_yield();
        
        printf("Main returned from thread_yield\n");
        if(thread_count <= 0) break;
        printf("count: %d\n", thread_count);
    }

    // We should never get here
    exit(0);
}

// This is the thread that gets started by thread_create
static void test_thread(void) {
    printf("In test_thread %d\n", thread->id);

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        
        if(rand()%2)
        {
            printf("Test_thread %d calling thread_create\n", thread->id);
            thread_create(&test_thread);
        }
        printf("Test_thread %d calling thread_yield\n", thread->id);
        
        thread_yield();
        
        printf("Test_thread %d returned from thread_yield\n", thread->id);
        if(rand()%2) break;
    }
    thread_exit(0);
    return;
}

// Yield to another thread
int thread_yield() {
    thread_ref old_thread = thread;
    
    // This is the scheduler, it is a bit primitive right now
    thread = thread->next;

    printf("Thread %d yielding to thread %d\n", old_thread->id, thread->id);
    printf("Thread %d calling swapcontext\n", old_thread->id);
    
    // This will stop us from running and restart the other thread
    //if(old_thread->id == 0 && thread->id == 0) thread_count = 0;
    swapcontext(&old_thread->ctx, &thread->ctx);
    
    // The other thread yielded back to us
    printf("Thread %d back in thread_yield\n", thread->id);
}

// Create a thread
int thread_create(int (*thread_function)(void)) {

    int newthread = inc_threads();
    thread_ref new = make_node();
    
    printf("Thread %d in thread_create\n", thread->id);
    
    printf("Thread %d calling getcontext and makecontext\n", thread->id);

    // First, create a valid execution context the same as the current one
    getcontext(&new->ctx);

    // Now set up its stack
    new->ctx.uc_stack.ss_sp = malloc(8192);
    new->ctx.uc_stack.ss_size = 8192;

    // This is the context that will run when this thread exits
    //new->ctx.uc_link = &thread->ctx;
    new->ctx.uc_link = &main_thread->ctx;

    // Now create the new context and specify what function it should run
    makecontext(&new->ctx, test_thread, 0);
    
    printf("Thread %d done with thread_create\n", thread->id);
}

// This doesn't do anything at present
void thread_exit(int status) {
    printf(">>Thread %d exiting\n", thread->id);
    thread->active = false;
    thread_count--;
    thread_ref oldthread = thread;
    thread_ref next_thread = oldthread->next;
    thread_ref prev_thread = oldthread->prev;
    prev_thread->next = next_thread;
    next_thread->prev = prev_thread;
    //thread_yield();
    //free(oldthread);
}
