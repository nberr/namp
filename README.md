# namp (Nico's Arduino Music Player)
a single process operating system running multiple threads including printing system information, reading from a SD card, and playing music. 
Here is an overview of the project
* img - a disk image of what the SD card should look like
* src - the source code for the project
  * io - user input/output
  * fs - file structure for EXT2 format
  * os - the underlying operating system
  * sd - code for reading the SD card with the arduino mega
  * sync - semaphore and mutex used for synching threads
  * globals.h - some global variables
  * namp.c - the main program
  
