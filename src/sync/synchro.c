#include <avr/io.h>
#include <avr/interrupt.h>

#include "../globals.h"
#include "../os/os.h"
#include "synchro.h"

#include <stdlib.h>
#include <string.h>

void mutex_init(mutex_t* m) {
	cli();
   // Initialize the mutex with currOwner = -1 ?
   m->currOwner = -1;
   m->numWaiting = 0;
   
   int i;
   for (i = 0; i < WLIST; i++) {
      m->waitlist[i] = -1;
   }
   sei();
}

void mutex_lock(mutex_t* m) {
	cli();
   if (m->currOwner != -1) {
      // Mutex already owned, put current thread on the waitlist
      m->waitlist[m->numWaiting] = get_current_thread(); //(Current thread);
      //m->waitlist[m->numWaiting] = sysArray.currThread; //(Current thread);
      m->numWaiting++;
      set_thread_state(get_current_thread(), THREAD_WAITING);
      yield();
      //sysArray.array[sysArray.currThread].state = THREAD_WAITING;
   }
   else {
      // This thread gets the mutex
      m->currOwner = get_current_thread(); // (Current thread)
   }
   sei();
}

void mutex_unlock(mutex_t* m) {
	cli();
   if (m->numWaiting == 0) {
      m->currOwner = -1;
   }
   else {
      // Give mutex to next thread on waiting list
      m->currOwner = m->waitlist[0];
      
      set_thread_state(m->currOwner, THREAD_READY);
      //sysArray.array[m->currOwner].state = THREAD_READY;
      // Shift mutex waitlist down
      shift(m->waitlist);
      m->numWaiting--;
   }
   sei();
}

void sem_init(semaphore_t* s, int16_t value) {
	cli();
   s->value = value;
   s->count = 0;
   uint8_t i;
   for (i = 0; i < WLIST; i++) {
      s->waitlist[i] = -1;
   }
   sei();
}

void sem_wait(semaphore_t* s) {
	cli();
   s->value--;
   
   if (s->value < 0) {
      s->waitlist[s->count++] = get_current_thread();
      
      // block the thread
      set_thread_state(get_current_thread(), THREAD_WAITING);
      yield();
      //sysArray.array[sysArray.currThread].state = THREAD_WAITING;
   }
   sei();
}

void sem_signal(semaphore_t* s) {
	cli();
   uint8_t woke;
   s->value++;
   
   if (s->value <= 0) {
      // remove a process from waitlist
      // wake up process (set to THREAD_READY)
      
      woke = s->waitlist[0];
      
      set_thread_state(woke, THREAD_READY);
      shift(s->waitlist);
   }
   sei();
}

void sem_signal_swap(semaphore_t* s) {
	cli();
   uint8_t woke;
   thread_t *oldThread;
   s->value++;

   if (s->value <= 0) {
   	  woke = s->waitlist[0];

   	  oldThread = get_thread_ptr(get_current_thread());
   	  oldThread->state = THREAD_READY;

   	  set_thread_state(woke, THREAD_RUNNING);
   	  shift(s->waitlist);

   	  context_switch(get_thread_ptr(woke), oldThread);
   }
   sei();
}

// helper functions
// shift waitlist down
void shift(int *arr) {
   int i;
   
   for (i = 0; i < 6; i++) {
      arr[i] = arr[i + 1];
   }
   arr[6] = -1;
}



