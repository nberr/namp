#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>

#include "io/serial.h"
#include "fs/ext2.h"
#include "fs/fs_util.h"
#include "sd/SdReader.h"
#include "globals.h"
#include "os/os.h"
#include "sync/synchro.h"
#include "namp.h"

super_block sb;

static song *songs;
static song *curr;
static uint32_t song_time = 0;
static uint32_t song_start = 0;

static uint8_t buffer[2][256];

static mutex_t locks[2];

static inode song_inode;

static uint32_t indirect[256];
static uint32_t dindirect[256];
static uint8_t count = 0;

uint16_t curr_block = 0;

int main(void) {
   uint8_t sd_card_status;
   dir_entry *entries;
   dir_entry *start;
   
   uint8_t *block;
   
   sd_card_status = sdInit(1);   //initialize the card with slow clock

   serial_init(); 
   clear_screen();   

   if (!sd_card_status) {
      set_cursor(1, 1);
      print_string("sd init failed...exiting");
      return 1;
   }

   block = calloc(sizeof(uint8_t), 1024);

   start_audio_pwm();
   os_init();

   /* read the super block */
   sdReadData(2, 0, (uint8_t *)&sb, sizeof(super_block));

   /* find the root inode */
   find_inode(&song_inode, (uint32_t)2);

   sdReadData(song_inode.i_block[0]*2, 0, (uint8_t *)block, (uint16_t)512);
   sdReadData(song_inode.i_block[0]*2 + 1, 0, (uint8_t *)&block[512], (uint16_t)512);
   entries = (void *)block;
   start = entries;

   /* skip ., .., and lost+found */
   entries = (void *)entries + entries->rec_len;
   entries = (void *)entries + entries->rec_len;
   entries = (void *)entries + entries->rec_len;
   
   songs = calloc(sizeof(song), 1);
   curr = songs;

   /* read the songs into the array */
   while ((void *)entries != (void *)start + 1024) {      
      curr->name = calloc(sizeof(char), entries->name_len + 1);
      strncpy(curr->name, entries->name, entries->name_len);
      
      curr->inode = entries->inode;
      
      /* find the size of the file */
      find_inode(&song_inode, curr->inode);

      curr->size = song_inode.i_size/20480;
      
      curr->num_blocks = curr->size / 1024;

      entries = (void *)entries + entries->rec_len;
      if ((void *)entries != (void *)start + 1024) {
         curr->next = calloc(sizeof(song), 1);
         curr->next->prev = curr;
         curr = curr->next;
      }
   }
   curr->next = songs;
   songs->prev = curr;
   
   curr = songs;
   
   /* free the block since we will be using the buffers now */
   free(block);

   /* initialize mutex and semaphores */
   mutex_init(&locks[0]);
   mutex_init(&locks[1]);

   /* create_threads */
   create_thread("playback", (uint16_t)playback, NULL, 64);
   create_thread("read",     (uint16_t)read,     NULL, 64);
   create_thread("display",  (uint16_t)display,  NULL, 64);
   //create_thread("idle",     (uint16_t)idle,     NULL, 16);

   /* start the os */
   os_start();

   clear_screen();
   set_cursor(1, 1);
   print_string("something went wrong!");

   return 0;
}

/*
 * play the audio from the buffer
 * thread 0
 */
void playback(void) {
   uint8_t idx = 0; /* which buffer to play */
   uint8_t pos = 0; /* where in the buffer  */

   while (1) {
      /* 
       * check if the entire buffer has been played
       * will overflow back to 0
       */
      if (pos == 0) {
         /* toggle which buffer to play */
         idx ^= 1;
      }

      mutex_lock(&locks[idx]);

      OCR2B = buffer[idx][pos];
      pos++;

      mutex_unlock(&locks[idx]);
   
      yield(); 
   }
}

/*
 * read the SD card and fill the buffer
 * thread 1
 */
void read(void) {
   uint8_t idx = 0;
   uint16_t curr_block = 0;

   uint32_t block;
   uint16_t offset;

   while (1) {
      /* toggle which buffer to store the data */
      idx ^= 1;
   
      mutex_lock(&locks[idx]);

      /* get the data and store it in the block */

      if (count == 4) {
         /* fetch the next block of data */

         count = 0;
         curr_block++;
         
      }
      else {
         /* use the current block */
         
         if (curr_block < 12) {
            block = song_inode.i_block[curr_block]*2;
            block += (count  >= 2) ? 1 : 0;

            offset = (count % 2) ? 256 : 0;

            sdReadData(block, offset, (uint8_t *)&buffer[idx], 256);

            count++;
         }
         else if (curr_block < 268) {
            /* single indirect blocks */
            
            /* read the indirect block once */
            if (curr_block == 12) {
               /* read in the indirect block */
               sdReadData(song_inode.i_block[12]*2, 0, (uint8_t *)&indirect[0], 512);
               sdReadData(song_inode.i_block[12]*2+1, 0, (uint8_t *)&indirect[128], 512);
            }

            block = indirect[curr_block - 12]*2;
            block += (count >= 2) ? 1 : 0;

            offset = (count % 2) ? 256 : 0;

            sdReadData(block, offset, (uint8_t *)&buffer[idx], 256);
         }
         else {
            /* double indirect block */
            
            /* read the 1st layer in once */
            if (curr_block == 268) {
               sdReadData(song_inode.i_block[13]*2, 0, (uint8_t *)&dindirect[0], 512);
               sdReadData(song_inode.i_block[13]*2+1, 0, (uint8_t *)&dindirect[128], 512);
            }

            if (!((curr_block - 268) % 256)) {
               /* fetch another block from 2nd layer */
               sdReadData(dindirect[(curr_block - 268) / 256]*2, 0, (uint8_t *)&indirect[0], 512);
               sdReadData(dindirect[(curr_block - 268) / 256]*2 + 1, 0, (uint8_t *)&indirect[128], 512);
            }  
         
            block = indirect[(curr_block - 268) % 256]*2;
            block += (count >= 2) ? 1 : 0;

            offset = (count % 2) ? 256 : 0;

            sdReadData(block, offset, (uint8_t *)&buffer[idx], 256);
         }
      }

      mutex_unlock(&locks[idx]);
   }
}

/*
 * display the information to the console
 * thread 2
 */
void display(void) {
   song_start = get_time();
   while (1) {
      
      handle_keys();
      
      set_color(YELLOW);
      set_cursor(1, 1);
      print_string("Program 5");
      
      /* print song info */
      set_cursor(2, 1);
      /* clear the line */
      print_string("                                                                            ");
      set_cursor(2, 1);
      print_string(curr->name);

      set_cursor(3, 1);
      print_string("song length: ");
      print_int32(curr->size);

      set_cursor(4, 1);
      print_string("                                     ");
      
      set_cursor(4, 1);
      print_string("current time: ");
      
      print_int32(get_time() - song_start);
      

      set_cursor(5, 1);
      set_color(32);
      print_string("Time: ");
      print_int32(get_time());
      
      set_cursor(6, 1);
      set_color(36);
      print_string("Number of threads: ");
      print_int(get_num_threads());
      
      //set_cursor(9, 1);
      //set_color(37);
      //print_thread_info();
   }
}

/*
 * take up extra time in cycle
 * thread 3
 */
void idle(void) {
   while (1);
}

/*
 * handle the key presses to change songs
 */
void handle_keys(void) {
   uint8_t input = read_byte();

   if (input != 255) {
      if (input == 'n') {
         /* go to next song */
         curr = curr->next;
         find_inode(&song_inode, curr->inode);
         song_start = get_time();
      }
      else if (input == 'p') {
         /* go to prev song */
         curr = curr->prev; 
         find_inode(&song_inode, curr->inode);
         song_start = get_time();
      }
   }
}

