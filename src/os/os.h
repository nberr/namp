#ifndef OS_H
#define OS_H

#include "../sync/synchro.h"

//This structure defines the register order pushed to the stack on a
//system context switch.
typedef struct regs_context_switch {
   uint8_t padding; //stack pointer is pointing to 1 byte below the top of the stack

   //Registers that will be managed by the context switch function
   uint8_t r29;
   uint8_t r28;
   uint8_t r17;
   uint8_t r16;
   uint8_t r15;
   uint8_t r14;
   uint8_t r13;
   uint8_t r12;
   uint8_t r11;
   uint8_t r10;
   uint8_t r9;
   uint8_t r8;
   uint8_t r7;
   uint8_t r6;
   uint8_t r5;
   uint8_t r4;
   uint8_t r3;
   uint8_t r2;
   uint8_t eind;
   uint8_t pch;
   uint8_t pcl;
} regs_context_switch;

//This structure defines how registers are pushed to the stack when
//the system 10ms interrupt occurs.  This struct is never directly
//used, but instead be sure to account for the size of this struct
//when allocating initial stack space
typedef struct regs_interrupt {
   uint8_t padding; //stack pointer is pointing to 1 byte below the top of the stack

   //Registers that are pushed to the stack during an interrupt service routine
   uint8_t r31;
   uint8_t r30;
   uint8_t r29;
   uint8_t r28;
   uint8_t r27;
   uint8_t r26;
   uint8_t r25;
   uint8_t r24;
   uint8_t r23;
   uint8_t r22;
   uint8_t r21;
   uint8_t r20;
   uint8_t r19;
   uint8_t r18;
   uint8_t rampz; //rampz
   uint8_t sreg; //status register
   uint8_t r0;
   uint8_t r1;
   uint8_t eind;
   uint8_t pch;
   uint8_t pcl;
} regs_interrupt;

typedef struct thread_t {
   uint8_t* stackPtr;
   uint8_t thread_id;
   char name[10];
   uint16_t func;
   uint8_t *stack;
   uint8_t *stackBase;
   uint16_t size;
   uint8_t sleepCount;
   uint16_t sched_count;
   thread_state state;
} thread_t;

typedef struct system_t {
   thread_t array[8];
   uint8_t threadsUsed;
   uint8_t currThread;
   uint32_t num_interrupts;
} system_t;

uint32_t get_time();
void os_start();
void os_init();
void create_thread(char *n, uint16_t address, void* args, uint16_t stack_size);
uint8_t get_num_threads();
uint8_t get_current_thread();
void set_thread_state(uint8_t thread, thread_state state);
thread_t *get_thread_ptr(uint8_t id);
void yield();

void print_thread_info(void);

#endif
