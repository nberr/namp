#ifndef SERIAL_H
#define SERIAL_H

void serial_init();
uint8_t byte_available();
uint8_t read_byte();
uint8_t write_byte(uint8_t b);
void print_string(uint8_t* s);
void print_int(uint16_t i);
void print_int32(uint32_t i);
void print_hex(uint16_t i);
void print_hex32(uint32_t i);
void set_cursor(uint8_t row, uint8_t col);
void set_color(uint8_t color);
void clear_screen(void);

#endif
