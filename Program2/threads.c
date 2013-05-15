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
#include <string.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

//static ucontext_t ctx[3];

static void test_thread(void);
void thread_exit(int);

struct thread_node
{
    int id;
    int index;
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
int refill_counter = 0;
#define REFILL (42)

struct thread_array
{
    thread_ref* array;
    int end;
    int size;
    bool empty;
    bool full;
    char* id;
};
typedef struct thread_array* tarray;
struct thread_queue
{
    thread_ref head;
    thread_ref tail;
    char* id;
};
typedef struct thread_queue* tqueue;
tarray ta;
tqueue tq;

tarray make_tarray(int size, char* s)
{
    tarray new = malloc(sizeof(struct thread_array));
    new->size = size;
    new->id = strdup(s);
    new->end = 0;
    new->array = malloc(sizeof(thread_ref)*size);
    new->empty = true;
    new->full = false;
    return new;
}

int insert_tarray(tarray tar, thread_ref thr)
{
    if(tar->full) return -1;//no space
    tar->array[tar->end] = thr;
    int ret = tar->end;
    thr->index = ret;
    tar->end++;
    if(tar->end == tar->size) tar->full = true;
    tar->empty = false;
    return ret;
}

thread_ref delete_tarray(tarray tar, int index)
{
    if(tar->empty) return NULL;
    thread_ref ret = tar->array[index];
    tar->array[index] = NULL;
    ret->index = -1;
    if(++index < tar->end)
    {
        int i;
        for(i = index; i < tar->end; i++)
        {
            tar->array[i-1] = tar->array[i];
            tar->array[i-1]->index = i-1;
        }
    }
    tar->end--;
    tar->full = false;
    if(tar->end == 0) tar->empty = true;
    return ret;
}

void insert_queue(thread_ref thr)
{
    if(tq->tail == NULL)
    {
        tq->head = thr;
        tq->tail = thr;
        thr->next = thr;
        thr->prev = thr;
    }
    else 
    {
        tq->tail->next = thr;
        thr->prev = tq->tail;
        tq->tail = thr;
    }
}

thread_ref dequeue(void)
{
    if(tq->head == NULL) return NULL;
    thread_ref ret = tq->head;
    tq->head = ret->next;
    if(tq->head != NULL)tq->head->prev = NULL;
    else tq->tail = NULL;
    ret->next = NULL;
    ret->prev = NULL;
}

void add_thread(thread_ref thr)
{
    if(!ta->full) insert_tarray(ta, thr);
    else insert_queue(thr);
}

thread_ref make_node(void)
{
    thread_ref new = malloc(sizeof(struct thread_node));
    new->id = threads;
    new->active = true;
    new->index = -1;
    //new->ctx;
    add_thread(new);
    /*if(thread == NULL)
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
    }*/
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
    ta = make_tarray(8, "tq");
    tq = malloc(sizeof(struct thread_queue));
    tq->head = NULL;
    tq->tail = NULL;
    tq->id = "tq";
    thread = make_node();
    main_thread = thread;
    insert_queue(delete_tarray(ta, 0));
    printf("Main calling thread_create\n");

    // Start one other thread
    thread_create(&test_thread);
    
    printf("Main returned from thread_create\n");

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        printf("Main calling thread_yield\n");
        
        //thread_yield();
        thread_schedule();
        
        printf("Main returned from thread_yield\n");
        if(thread_count <= 0) break;
        printf("count: %d\n", thread_count);
    }
    printf("All threads finished.\n");
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
        
        //thread_yield();
        thread_schedule();
        
        printf("Test_thread %d returned from thread_yield\n", thread->id);
        if(rand()%2) break;
    }
    thread_exit(0);
    return;
}

void print_threads(void)
{
    int i;
    printf("ta::\n");
    for(i = 0; i < ta->end; i++)
    {
        printf("\tta:thread: %d\n", ta->array[i]->id);
    }
    printf("tq::\n");
    thread_ref thr = tq->head;
    while(thr != NULL)
    {
        printf("\ttq:thread: %d\n", thr->id);
        thr = thr->next;
    }
}

int thread_switch(thread_ref next)
{
    thread_ref old_thread = thread;
    
    thread = next;

    printf("Thread %d switching to thread %d\n", old_thread->id, thread->id);
    printf("Thread %d calling swapcontext\n", old_thread->id);
    
    // This will stop us from running and restart the other thread
    //if(old_thread->id == 0 && thread->id == 0) thread_count = 0;
    swapcontext(&old_thread->ctx, &thread->ctx);
}

thread_ref lotto(tarray tar)
{
    int index = rand()%tar->end;
    if(tar->array[index]!=NULL) 
        return delete_tarray(ta, index);
    else return NULL;
}

int resize_array(tarray tar, int size)
{
    resize = true;
    if(size < tar->end) return -1;
    tar->array = realloc(tar->array, sizeof(thread_ref)*size);
    return 1;
}

void refill_array(tarray tar)
{
    refill_counter = 0;
    if(thread_count >= 4 * tar->size)
    {
        resize_array(tar, tar->size * 2);
    }
    else if(tar->size > 8 && thread_count < tar->size / 2)
    {
        resize_array(tar, tar->size / 2);
    }
    while(!tar->full)
    {
        thread_ref thr = dequeue();
        if(thr == NULL) break;
        insert_tarray(tar, thr);
    }
}

int thread_schedule(void)
{
    thread_ref old_thread = thread;
    thread_ref next;
    if(!ta->empty) next = lotto(ta);
    else next = dequeue();
    insert_queue(next);
    if(refill_counter++ >= REFILL)
    {
        refill_array(ta);
    }
    
    printf("Schedule switch: %d -> %d\n", old_thread->id, next->id);
    if(next==NULL) return -1;
    thread_switch(next);
    return 1;
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
    if(thread->index >= 0)
    {
        delete_tarray(ta, thread->index);
    }
}
