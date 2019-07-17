#include <avr/io.h>

#include "serial.h"
#include "../globals.h"

/*
 * Initialize the serial port.
 */
void serial_init() {
   uint16_t baud_setting;

   UCSR0A = _BV(U2X0);
   baud_setting = 16; //115200 baud

   // assign the baud_setting
   UBRR0H = baud_setting >> 8;
   UBRR0L = baud_setting;

   // enable transmit and receive
   UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

/*
 * Return 1 if a character is available else return 0.
 */
uint8_t byte_available() {
   return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

/*
 * Unbuffered read
 * Return 255 if no character is available otherwise return available character.
 */
uint8_t read_byte() {
   if (UCSR0A & (1 << RXC0)) return UDR0;
   return 255;
}

/*
 * Unbuffered write
 *
 * b byte to write.
 */
uint8_t write_byte(uint8_t b) {
   //loop until the send buffer is empty
   while (((1 << UDRIE0) & UCSR0B) || !(UCSR0A & (1 << UDRE0))) {}

   //write out the byte
   UDR0 = b;
   return 1;
}

void print_string(uint8_t* s) {
   while(*s) {
      write_byte(*s++);
   }
}

void print_int(uint16_t i) {
   uint16_t rest;
   if(i > 9) { 
      rest = i / 10;
      i -= 10 * rest;
      print_int(rest);
   }
   write_byte('0'+i);
}

uint8_t send_int(uint16_t x) {
   uint8_t i;
   uint16_t base = 9999;
   uint8_t leading = 1;
   uint8_t digit;

   if (x == 0) {
      write_byte(48);
      return 1;
   }

   for (i=0;i<5;i++) {
      digit = x / (base+1);

      if (digit != 0)
         leading = 0;

      if (x > base) {
         if (!leading)
            write_byte(48 + digit);
         x = x % (base+1);
      } else {
         if (!leading)
            write_byte(48 + digit);
      }

      base = base / 10;
   }

   return x;
}

void print_int32(uint32_t i) {
   uint32_t rest;
   if(i > 9) { 
      rest = i / 10;
      i -= 10 * rest;
      print_int(rest);
   }
   write_byte('0'+i);
}

void print_hex(uint16_t i) {
   uint16_t rest;
   if(i>15) {
      rest = i / 16;
      i -= 16 * rest;
      print_hex(rest);
   }
   if(i<10)
      write_byte('0'+i);
   else
      write_byte('A'+i-10);
}

void print_hex32(uint32_t i) {
   uint32_t rest;
   if(i>15) {
      rest = i / 16;
      i -= 16 * rest;
      print_hex32(rest);
   }
   if(i<10)
      write_byte('0'+i);
   else
      write_byte('A'+i-10);
}

void set_cursor(uint8_t row, uint8_t col) {
   write_byte(0x1B);
   write_byte('[');
   print_int((uint16_t)row);
   write_byte(';');
   print_int((uint16_t)col);
   write_byte('H');

}

void hide_cursor() {
   write_byte(0x1B);
   write_byte('[');
   write_byte('?');
   write_byte('2');
   write_byte('5');
   write_byte('h');

}

void set_color(uint8_t color) {
   write_byte(0x1B);
   write_byte('[');
   print_int((uint16_t)color);
   write_byte('m');
}

void clear_screen(void) {
   write_byte(0x1B);
   write_byte('[');
   write_byte('2');
   write_byte('J');
}
