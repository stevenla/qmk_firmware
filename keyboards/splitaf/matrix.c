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

#ifndef DEBOUNCE
#   define DEBOUNCE	5
#endif

#define print_matrix_header()  print("\nr/c 0123456789ABCDEF\n")
#define print_matrix_row(row)  print_bin_reverse16(matrix_get_row(row))
#define matrix_bitpop(i)     bitpop16(matrix[i])
#define ROW_SHIFTER ((uint16_t)1)

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];

// Debouncing: store for each key the number of scans until it's eligible to
// change.  When scanning the matrix, ignore any changes in keys that have
// already changed in the last DEBOUNCE scans.
static uint8_t debounce_matrix[MATRIX_ROWS * MATRIX_COLS];

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

void matrix_init(void) {
  // initialize row and col
  unselect_rows();
  init_cols();

  // initialize matrix state: all keys off
  for (uint8_t i=0; i < MATRIX_ROWS; i++) {
    matrix[i] = 0;
    for (uint8_t j=0; i < MATRIX_COLS; j++) {
      debounce_matrix[i * MATRIX_COLS + j] = 0;
    }
  }

  matrix_init_quantum();
}


// Returns a matrix_row_t whose bits are set if the corresponding key should be
// eligible to change in this scan.
matrix_row_t debounce_mask(uint8_t row) {
  matrix_row_t result = 0;
  for (uint8_t j=0; j < MATRIX_COLS; ++j) {
    if (debounce_matrix[row * MATRIX_COLS + j]) {
      --debounce_matrix[row * MATRIX_COLS + j];
    } else {
      result |= (1 << j);
    }
  }
  return result;
}

// Report changed keys in the given row.  Resets the debounce countdowns
// corresponding to each set bit in 'change' to DEBOUNCE.
void debounce_report(matrix_row_t change, uint8_t row) {
  for (uint8_t i = 0; i < MATRIX_COLS; ++i) {
    if (change & (1 << i)) {
      debounce_matrix[row * MATRIX_COLS + i] = DEBOUNCE;
    }
  }
}


uint8_t matrix_scan(void)
{
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    select_row(i);
    wait_us(30);  // wait for a stable read value
    matrix_row_t mask = debounce_mask(i);
    matrix_row_t cols = (read_cols(i) & mask) | (matrix[i] & ~mask);
    debounce_report(cols ^ matrix[i], i);
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
 *   pin:
 *   col: 0   1   2   3   4   5   6   7   8
 *   pin:
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
  return master_row;

  // SLAVE
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
}
