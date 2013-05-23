//
//  threads.c
//  
//	Justin Yeo, Hai Vo, Erik Swedberg 
//  this is a linux-base thread scheduler 

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <limits.h>

#define _XOPEN_SOURCE
#include <ucontext.h>

static void test_thread(void);
void thread_exit(int);


static int threads = 0;//thread id counter
#define THR_LIMIT (1024)//limit for thread id counter
static int thread_count = 0;//number of live threads

int inc_threads(void)
{
    thread_count++;
    return ++threads;
}


int refill_counter = 0;//counter used to tell when to refill the active array
#define REFILL (42)//refill_counter's limit
bool scheduled = false;


void handle_alarm(int sig)
{
    //printf(")> Timer <(\n");//debug statement
    thread_schedule();
}

struct itimerval* thread_timer;
void create_timer(struct itimerval *value, long time)
{
    value = malloc(sizeof(struct itimerval));
    value->it_interval.tv_sec = 0;//seconds//next value
    value->it_interval.tv_usec = time;//microseconds
    value->it_value.tv_sec = 0;//current value
    value->it_value.tv_usec = time;
    setitimer(ITIMER_VIRTUAL, value, NULL);
    signal(SIGVTALRM, handle_alarm);
}


/*
thread_refs are what hold each threads ucontetx_t, and are set up so
they know where in an array they are, when they need to be removed,
and also a next field so they can be used in a link list.
*/

struct thread_node
{
    int id;//debug info
    int index;//index when in active array, -1 when in queue
    struct thread_node* next;//points to next item in wating queue
    struct thread_node* prev;
    ucontext_t ctx;
    bool active;//not strictly necessary
};
typedef struct thread_node* thread_ref;

void add_thread(thread_ref);
thread_ref make_node(void)
{
    thread_ref new = malloc(sizeof(struct thread_node));
    new->id = threads;
    new->active = true;
    new->index = -1;
    add_thread(new);
    return new;
}

/*
tarray is the structure that holds the active thread_refs.
*/

struct thread_array
{
    thread_ref* array;
    int end;//index after last element of array
    int size;//capacity of array
    bool empty;
    bool full;
    char* id;//debug info
};
typedef struct thread_array* tarray;

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

/*
tqueue is the structure that holds the waiting thread_refs.
*/

struct thread_queue
{
    thread_ref head;
    thread_ref tail;
    char* id;//debug info
};
typedef struct thread_queue* tqueue;

void insert_queue(tqueue que, thread_ref thr)
{
    if(que->tail == NULL)
    {
        que->head = thr;
        que->tail = thr;
        thr->next = thr;
        thr->prev = thr;
    }
    else 
    {
        que->tail->next = thr;
        thr->prev = que->tail;
        que->tail = thr;
    }
}

thread_ref dequeue(tqueue que)
{
    if(que->head == NULL) return NULL;
    thread_ref ret = que->head;
    que->head = ret->next;
    if(que->head != NULL)que->head->prev = NULL;
    else que->tail = NULL;
    ret->next = NULL;
    ret->prev = NULL;
}

void empty_queue(tqueue que)
{
	thread_ref curr = que->head;
	que->head = NULL;
	que->tail = NULL;
}

tarray ta;//active array
tqueue tq;//waiting queue
tqueue inactive;

//pulls in threads from tq until tarray is full
void refill_array(tarray tar)
{
    refill_counter = 0;
    while(!tar->full)
    {
        thread_ref thr = dequeue(tq);
        if(thr == NULL) break;
        insert_tarray(tar, thr);
    }
}


void add_thread(thread_ref thr)
{
	if(!ta->full) insert_tarray(ta, thr);
	else insert_queue(tq, thr);
}


thread_ref thread = NULL;//active thread
thread_ref main_thread = NULL;//pointer to main's thread

void fib_thread(void)
{
    printf("In fib_thread %d\n", thread->id);
    int n = rand()%512;
    printf("fib_thread %d finding fib(%d)\n", thread->id, n);
    
    if(n%2)
    {
        printf("fib_thread %d calling thread_create\n", thread->id);
        thread_create(&fib_thread);
    }
    int j;
    unsigned long next;
    //calculate the fibonacci number giving the thread a process to run so the scheduler can do its job
    for(j = 0; j <= n * rand()%512; j++){
        unsigned long i, first = 0, second = 1;
        next = 0;
        printf("fib_thread %d : %lu\n",thread->id, next);
        for(i = 0; i<=n ; i++){
            next = first + second;
            first = second;
            second = next;  
        }
    }
    printf("fib_thread %d finished with %lu\n", thread->id, next);
    thread_exit(0);
    while(!thread->active){}
}

// This is the main thread
// In a real program, it should probably start all of the threads and then wait for them to finish
// without doing any "real" work

int main(void) {
    srand(time(NULL));
    printf("Main starting\n");
    ta = make_tarray(8, "tq");
    tq = malloc(sizeof(struct thread_queue));
    inactive = malloc(sizeof(struct thread_queue));
    tq->head = NULL;
    tq->tail = NULL;
    tq->id = "tq";
    inactive->head = NULL;
    inactive->tail = NULL;
    inactive->id = "inactive";
    thread = make_node();
    main_thread = thread;
    insert_queue(tq, delete_tarray(ta, 0));
    printf("Main calling thread_create\n");

    // Start one other thread
    //thread_create(&test_thread);
    int i;
    for(i=0; i<8; i++){
    	thread_create(&fib_thread);
	}
    
    printf("Main returned from thread_create\n");
    create_timer(thread_timer, 10);

    // Loop, doing a little work then yielding to the other thread
    while(1) {
        if(scheduled)
        {
            scheduled = false;
            int rand_num = rand() % 100;
            if(rand_num > 25)
            {
                printf("Main calling thread_create\n");
                thread_create(&fib_thread);
                printf("Main returned from thread_create\n");
            }
            if(thread_count <= 0) break;
        }
    }
    printf("%d threads finished.\n", threads);
    exit(0);
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

int thread_schedule(void)
{
	scheduled = true;
	empty_queue(inactive);
	thread_ref old_thread = thread;
	thread_ref next;
	if(!old_thread->active) insert_queue(inactive, old_thread);
	if(!ta->empty) next = lotto(ta);
	else next = dequeue(tq);
	insert_queue(tq, next);
	if(++refill_counter >= REFILL)
	{
		refill_array(ta);
	}
	printf("Schedule switch: %d -> %d\n", old_thread->id, next->id);
	if(next==NULL) return -1;
	thread_switch(next);
	return 1;
}

// Create a thread
int thread_create(void (*thread_function)(void)) {

    //if(threads == INT_MAX) return -1;
    if(threads >= THR_LIMIT) return -1;
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
    makecontext(&new->ctx, thread_function, 0);
    
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
    else
    {
    	thread->prev->next = NULL;
    	tq->tail = thread->prev;
    	thread->next = NULL;
    	thread->prev = NULL;

    }
    //thread_schedule();
}
