#ifndef NAMP_H
#define NAMP_H

typedef struct song {
   uint32_t inode;
   char *name;
   struct song *next;
   struct song *prev;
   uint32_t size;
   uint16_t num_blocks;
} song;

/* main threads */
void display(void);
void playback(void);
void read(void);
void idle(void);

/* helper functions */
void handle_keys(void);

#endif

