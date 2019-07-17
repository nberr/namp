#ifndef SYNCHRO_H
#define SYNCHRO_H

#define WLIST 7

typedef enum thread_state {THREAD_RUNNING, THREAD_READY, THREAD_SLEEPING,
                   THREAD_WAITING} thread_state;

typedef struct mutex_t {
   int currOwner;
   int waitlist[7];
   int numWaiting;
} mutex_t;

typedef struct semaphore_t {
   int value; // comparing and swapping
   int waitlist[7];
   int count;
} semaphore_t;

// function prototypes

// mutex functions
void mutex_init(struct mutex_t* m);
void mutex_lock(struct mutex_t* m);
void mutex_unlock(struct mutex_t* m);

// semaphore functions
void sem_init(struct semaphore_t* s, int16_t value);
void sem_wait(struct semaphore_t* s);
void sem_signal(struct semaphore_t* s);
void sem_signal_swap(struct semaphore_t* s);

// helper function
void shift(int *arr);  // shift the queue down

#endif
