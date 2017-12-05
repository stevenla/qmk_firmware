/*
Copyright 2012-2017 Jun Wako, Jack Humbert

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdint.h>
#include <stdbool.h>
#if defined(__AVR__)
#include <avr/io.h>
#endif
#include "wait.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "timer.h"
#include "i2c.h"

#ifndef DEBOUNCING_DELAY
  #define DEBOUNCING_DELAY 5
#endif

#define print_matrix_header()  print("\nr/c 0123456789ABCDEF\n")
#define print_matrix_row(row)  print_bin_reverse16(matrix_get_row(row))
#define matrix_bitpop(i)     bitpop16(matrix[i])
#define ROW_SHIFTER ((uint16_t)1)

#define i2c(status, debug) do { dprint(debug); if (status > 0) { dprint(" FAILED\n"); return; } else { dprint("\n"); } } while (0)

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];

static void init_cols(void);
static matrix_row_t read_cols(uint8_t row);
static void unselect_rows(void);
static void select_row(uint8_t row);

__attribute__ ((weak))
void matrix_init_quantum(void) {
  matrix_init_kb();
}

__attribute__ ((weak))
void matrix_scan_quantum(void) {
  matrix_scan_kb();
}

__attribute__ ((weak))
void matrix_init_kb(void) {
  matrix_init_user();
}

__attribute__ ((weak))
void matrix_scan_kb(void) {
  matrix_scan_user();
}

__attribute__ ((weak))
void matrix_init_user(void) {
}

__attribute__ ((weak))
void matrix_scan_user(void) {
}

inline
uint8_t matrix_rows(void) {
  return MATRIX_ROWS;
}

inline
uint8_t matrix_cols(void) {
  return MATRIX_COLS;
}

// ----------------------------------------
// TWI stuff start
// ----------------------------------------


// register addresses (see "mcp23018.md")
#define IODIRA 0x00  // i/o direction register
#define IODIRB 0x01
#define GPPUA  0x0C  // GPIO pull-up resistor register
#define GPPUB  0x0D
#define GPIOA  0x12  // general purpose i/o port register (write modifies OLAT)
#define GPIOB  0x13
#define OLATA  0x14  // output latch register
#define OLATB  0x15

// TWI aliases
#define TWI_ADDRESS 0b0100000
#define TWI_ADDR_WRITE ( (TWI_ADDRESS<<1) | I2C_WRITE )
#define TWI_ADDR_READ  ( (TWI_ADDRESS<<1) | I2C_READ  )

void init_remote(void) {
  dprintln("init_remote: start");
  i2c_master_init();
  dprintln("init_remote: init");

  // set pin direction
  // - unused  : input  : 1
  // - input   : input  : 1
  // - driving : output : 0
  // if(i2c_master_start(TWI_ADDR_WRITE) > 0) {
  //   dprintln("init_remote: dir start FAILED");
  //   return;
  // }
  i2c(i2c_master_start(TWI_ADDR_WRITE), "init_remote: dir start");
  i2c(i2c_master_write(IODIRA), "init_remote: dir set ptr"); // set pointer
  i2c(i2c_master_write(0b11111111), "init_remote: dir write first byte");  // IODIRA
  //                     I__OOOOO
  i2c(i2c_master_write(0b11100000), "init_remote: dir write second byte");  // IODIRB
  i2c_master_stop();
  dprintln("init_remote: dir stop");

	// set pull-up
	// - unused  : on  : 1
	// - input   : on  : 1
	// - driving : off : 0
  i2c(i2c_master_start(TWI_ADDR_WRITE), "init_remote: pu start");
  i2c(i2c_master_write(GPPUA), "init_remote: pu set ptr");  // set pointer
  i2c(i2c_master_write(0b11111111), "init_remote: pu write first byte");  // GPPUA
  i2c(i2c_master_write(0b11100000), "init_remote: pu write second byte");  // GPPUB
  i2c_master_stop();
  dprintln("init_remote: pu stop");

  // set logical value (doesn't matter on inputs)
  // - unused  : hi-Z : 1
  // - input   : hi-Z : 1
  // - driving : hi-Z : 1
  i2c(i2c_master_start(TWI_ADDR_WRITE), "init_remote: val start");
  i2c(i2c_master_write(OLATA), "init_remote: val set ptr");  // set pointer
  i2c(i2c_master_write(0b11111111), "init_remote: val write first byte");  // OLATA
  i2c(i2c_master_write(0b11111111), "init_remote: val write second byte");  // OLATB
  i2c_master_stop();
  dprintln("init_remote: val stop");
}


// ----------------------------------------
// TWI stuff end
// ----------------------------------------

void matrix_init(void) {
  debug_enable = true;
  if (debug_enable) {
    wait_us(1000000);
  }
  dprintln("matrix_init: start");
  // initialize row and col
  init_remote();
  dprintln("matrix_init: slave");
  unselect_rows();
  dprintln("matrix_init: unselect_rows");
  init_cols();
  dprintln("matrix_init: cols");

  // initialize matrix state: all keys off
  for (uint8_t i=0; i < MATRIX_ROWS; i++) {
    matrix[i] = 0;
  }
  dprintln("matrix_init: done");

  matrix_init_quantum();
}

uint8_t matrix_scan(void)
{
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    select_row(i);
    wait_us(30);  // wait for a stable read value
    // TODO: debouncing
    matrix_row_t cols = read_cols(i);
    matrix[i] = cols;
    unselect_rows();
  }

  matrix_scan_quantum();
  return 1;
}

/**
 * @deprecated not called, apparently
 */
bool matrix_is_modified(void)
{
  return true;
}

inline
bool matrix_is_on(uint8_t row, uint8_t col)
{
  return (matrix[row] & ((matrix_row_t)1<col));
}

inline
matrix_row_t matrix_get_row(uint8_t row)
{
  // Matrix mask lets you disable switches in the returned matrix data. For example, if you have a
  // switch blocker installed and the switch is always pressed.
  return matrix[row];
}

void matrix_print(void)
{
  print_matrix_header();

  for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
    phex(row); print(": ");
    print_matrix_row(row);
    print("\n");
  }
}

uint8_t matrix_key_count(void)
{
  uint8_t count = 0;
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    count += matrix_bitpop(i);
  }
  return count;
}


/* ----------------------------------------
 * Helper functions start here
 *
 * MASTER:
 *   row: 0   1   2   3   4
 *   pin: B5  B4  E6  D7  C6
 *   col: 0   1   2   3   4   5   6
 *   pin: F4  F5  F6  F7
 *
 * SLAVE:
 *   row: 0   1   2   3   4
 *   pin: B0  B1  B2  B3  B4
 *   col: 7   8   9   10  11  12  13  14  15
 *   pin: A0  A1  A2  A3  A4  A5  A6  A7  B7
 *
 * ---------------------------------------- */

/**
 * Set DDR=0 (input) PORT=1 (pullup) to select.
 */
static void init_cols(void)
{
  // MASTER  76543210
   DDRF &= 0b00001111;
  PORTF |= 0b11110000;

  // SLAVE
}

static matrix_row_t read_cols(uint8_t row)
{
  // MASTER
  matrix_row_t master_row = (
    (PINF&(1<<4) ? 0 : (1<<0)) |
    (PINF&(1<<5) ? 0 : (1<<1)) |
    (PINF&(1<<6) ? 0 : (1<<2)) |
    (PINF&(1<<7) ? 0 : (1<<3))
  );

  // SLAVE
  i2c_master_start(TWI_ADDR_WRITE);
  i2c_master_write(GPIOA);
  i2c_master_start(TWI_ADDR_READ);
  uint8_t dataA = i2c_master_read(1);
  uint8_t dataB = i2c_master_read(1);
  i2c_master_stop();
  matrix_row_t slave_row = (
    (~dataA & 0b11111111) |
    ((~dataB & 0b10000000) << 1)
  );

  matrix_row_t both_rows = (slave_row << 7 | master_row);
  return both_rows;

  // print_bin8(~dataA);
  // print("\n");
  // print_bin16(dataA);
  // print("\n");
  // print_bin16(master_row | (keysDownA << 7) | (keysDownB << 15));
  // print_bin16(both_rows);
  // print("\n");
  // return master_row | ((~dataA) << 7);
  // return master_row;
  // return master_row | (keysDownA << 7) | (keysDownB << 15);
}

/**
 * Set DDR=0 (input) PORT=0 (pulldown) to select.
 */
static void unselect_rows(void)
{
  // MASTER  76543210
   DDRB &= 0b11001111;
  PORTB &= 0b11001111;
   DDRE &= 0b10111111;
  PORTE &= 0b10111111;
   DDRD &= 0b01111111;
  PORTD &= 0b01111111;
   DDRC &= 0b11101111;
  PORTC &= 0b11101111;

  // SLAVE
  // set all rows hi-Z : 1
  i2c(i2c_master_start(TWI_ADDR_WRITE), "unselect_rows: start");
  i2c(i2c_master_write(GPIOB), "unselect_rows: set ptr");
  i2c(i2c_master_write(0xFF), "unselect_rows: write");
  i2c_master_stop();
  dprintln("unselect_rows: stop");
}

/**
 * Set DDR=1 (output) PORT=0 (low) to select.
 */
static void select_row(uint8_t row)
{
  // MASTER
  switch (row) {
    case 0:
       DDRB |=   1<<5;
      PORTB &= ~(1<<5);
      break;
    case 1:
       DDRB |=   1<<4;
      PORTB &= ~(1<<4);
      break;
    case 2:
       DDRE |=   1<<6;
      PORTE &= ~(1<<6);
      break;
    case 3:
       DDRD |=   1<<7;
      PORTD &= ~(1<<7);
      break;
    case 4:
       DDRC |=   1<<6;
      PORTC &= ~(1<<6);
      break;
  }

  // SLAVE
  // set active row low  : 0
  // set other rows hi-Z : 1
  i2c(i2c_master_start(TWI_ADDR_WRITE), "select_row: start");
  i2c(i2c_master_write(GPIOB), "select_row: set ptr");
  i2c(i2c_master_write(0xFF & ~(1 << row)), "select_row: write row");
  i2c_master_stop();
}
