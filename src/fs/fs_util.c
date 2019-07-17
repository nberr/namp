#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcmp */

#include "fs_util.h"
#include "ext2.h"
#include "../globals.h"
#include "../sd/SdReader.h"

#define MAX_OFFSET 512

extern super_block sb;

void print_usage(void) {
   fprintf(stderr, "usage: ext2reader image [[-l] path]\n");
   fprintf(stderr, "\tl: print to the screen the contents of path\n");
}

/*
 * find the inode based on the inode number
 */
void find_inode(inode *data, uint32_t inode_number) {
   /* https://wiki.osdev.org/Ext2 */
   
   uint32_t block_group      = (inode_number - 1) / sb.s_inodes_per_group;
   uint32_t index            = (inode_number - 1) % sb.s_inodes_per_group;
   uint32_t containing_block = (index * sizeof(inode)) / MAX_OFFSET;
   uint16_t offset           = index * sizeof(inode) % MAX_OFFSET;
   uint32_t block            = BASE_BLOCKS +
                               (2*sb.s_blocks_per_group * block_group) +
                               containing_block;

   sdReadData(block, offset, (uint8_t *)data, 128);
}

void traverse(inode *data, char *path) {
   
   dir_entry *entry;
   uint8_t block[BLOCK_SIZE];
   uint8_t curr_block = 0;
   uint8_t found = 0;
   
   char *str = calloc(sizeof(char), strlen(path));
   strncpy(str, path, strlen(path));

   const char delim[2] = ROOT_PATH;
   char *token;
   char *name;

   token = strtok(str, delim);

   while (token != NULL) {
      
      sdReadData(data->i_block[curr_block]*2, 0, (uint8_t *)block, BLOCK_SIZE);
      entry = (dir_entry *)block;

      while ((void *)entry != (void *)&block[BLOCK_SIZE]) {
         name = calloc(sizeof(char), entry->name_len);
         strncpy(name, entry->name, entry->name_len);

         if (!(strcmp(token, name))) {
            find_inode(data, entry->inode);
            found = 1;
            
            break;
         }

         free(name);
         
         entry = (void *)entry + entry->rec_len;
      }

      if (!found) {
         fprintf(stderr, "%s not found\n", path);
         exit(EXIT_FAILURE);
      }

      token = strtok(NULL, delim);
   }
   free(str);
}

void print_dir(inode data) {
   uint8_t block[BLOCK_SIZE];
   uint8_t curr_block = 0;
   uint8_t s_indirect[BLOCK_SIZE];
   uint8_t d_indirect[BLOCK_SIZE];
   uint16_t curr_addr;
   uint16_t curr_i_addr;
   uint32_t *address;
   uint32_t *i_address;
   inode meta;
   dir_entry *entry;
   
   /* check the direct blocks */
   for (curr_block = 0; curr_block < EXT2_NDIR_BLOCKS; curr_block++) {
      sdReadData(data.i_block[curr_block]*2, 0, (uint8_t *)block, BLOCK_SIZE);
      entry = (void *)block;
      
      while (entry->inode) {
         print_name(entry->name, entry->name_len);
         find_inode(&meta, entry->inode);
         print_meta_data(meta);
         
         entry = (void *)entry + entry->rec_len;
         
         if ((void *)entry == (void *)&block[1024]) {
            
         }
      }
   }
   
   if (data.i_size < (BLOCK_SIZE*EXT2_NDIR_BLOCKS)) {
    
      return;
   }
   
   /* check the singly indirect blocks */
   sdReadData(data.i_block[EXT2_NDIR_BLOCKS]*2, 0, (uint8_t *)s_indirect, BLOCK_SIZE);
   address = (void *)s_indirect;
   
   for (curr_addr = 0; curr_addr < ADDR_PER_BLOCK; curr_addr++) {
      sdReadData(address[curr_addr]*2, 0, (uint8_t *)block, BLOCK_SIZE);
      
      entry = (void *)block;
      while ((void *)entry != (void *)&block[BLOCK_SIZE]) {
         print_name(entry->name, entry->name_len);
         find_inode(&meta, entry->inode);
         print_meta_data(meta);
         
         entry = (void *)entry + entry->rec_len;
      }
   }
   
   /* check the doubly indirect blocks */
   sdReadData(data.i_block[EXT2_DIND_BLOCK]*2, 0, (uint8_t *)d_indirect, BLOCK_SIZE);
   i_address = (void *)d_indirect;
   
   for (curr_i_addr = 0; curr_i_addr < ADDR_PER_BLOCK; curr_i_addr++) {
      sdReadData(i_address[curr_i_addr]*2, 0, (uint8_t *)s_indirect, BLOCK_SIZE);
      address = (void *)s_indirect;
      
      for (curr_addr = 0; curr_addr < ADDR_PER_BLOCK; curr_addr++) {
         sdReadData(address[curr_addr]*2, 0, (uint8_t *)block, BLOCK_SIZE);
         
         entry = (void *)block;
         while ((void *)entry != (void *)&block[BLOCK_SIZE]) {
            print_name(entry->name, entry->name_len);
            find_inode(&meta, entry->inode);
            print_meta_data(meta);
            
            entry = (void *)entry + entry->rec_len;
         }
      }
   }
   
}

/*
 * prints the name without having to null terminate the string
 * NOTE: formatting will break if the name is longer than 20 characters
 */
void print_name(char *name, uint16_t name_len) {
   uint8_t spaces = NAME_SEP;
   uint8_t i;

   for (i = 0; i < name_len; i++, spaces--) {
      printf("%c", name[i]);
   }

   while (spaces--) {
      printf(" ");
   }
}

void print_reg(inode data) {
   uint8_t block[BLOCK_SIZE];
   uint8_t curr_block = 0;
   uint8_t s_indirect[BLOCK_SIZE];
   uint8_t d_indirect[BLOCK_SIZE];
   uint16_t curr_addr;
   uint16_t curr_i_addr;
   uint32_t *address;
   uint32_t *i_address;

   /* check the direct blocks */
   uint16_t i;
   for (curr_block = 0; curr_block < EXT2_NDIR_BLOCKS; curr_block++) {
      sdReadData(data.i_block[curr_block]*2, 0, (uint8_t *)block, BLOCK_SIZE);

      for (i = 0; i < BLOCK_SIZE; i++) {
         printf("%c", block[i]);
      }
   }
   
   /* file was contained within the 12 direct blocks */
   if (data.i_size < (BLOCK_SIZE*EXT2_NDIR_BLOCKS)) {
      return;
   }

   /* check the singly indirect blocks */
   sdReadData(data.i_block[EXT2_DIND_BLOCK]*2, 0, (uint8_t *)s_indirect, BLOCK_SIZE);
   address = (void *)s_indirect;

   for (curr_addr = 0; curr_addr < ADDR_PER_BLOCK; curr_addr++) {
      sdReadData(address[curr_addr]*2, 0, (uint8_t *)block, BLOCK_SIZE);

      for(i = 0; i < BLOCK_SIZE; i++) {
         printf("%c", block[i]);
      }
   }
   
   /* check the doubly indirect blocks */
   sdReadData(data.i_block[EXT2_DIND_BLOCK]*2, 0, (uint8_t *)d_indirect, BLOCK_SIZE);
   i_address = (void *)d_indirect;

   for (curr_i_addr = 0; curr_i_addr < ADDR_PER_BLOCK; curr_i_addr++) {
      sdReadData(i_address[curr_i_addr]*2, 0, (uint8_t *)s_indirect, BLOCK_SIZE);
      address = (void *)s_indirect;

      for (curr_addr = 0; curr_addr < ADDR_PER_BLOCK; curr_addr++) {
         sdReadData(address[curr_addr]*2, 0, (uint8_t *)block, BLOCK_SIZE);

         for (i = 0; i < BLOCK_SIZE; i++) {
            printf("%c", block[i]);
         }
      }
   }

   /*no support for TIB */
}



/*
 * Prints the meta data for the inode (size and type)
 */
void print_meta_data(inode meta) {
   if ((meta.i_mode & MODE_MASK) == EXT2_S_IFDIR) {
      
      /* no size for directories */
      printf("       0");
      
      /* print type */
      printf("        D\n");
   }
   else if ((meta.i_mode & MODE_MASK) == EXT2_S_IFREG) {
      
      /* print size */
      printf("%8d", meta.i_size);
      
      /* print type */
      printf("        F\n");
   }
   else {
      printf("%d", meta.i_size);
      printf("invalid type 0x%04X\n", meta.i_mode & MODE_MASK);
   }
}

