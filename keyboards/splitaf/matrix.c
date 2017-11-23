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


/* Set 0 if debouncing isn't needed */

#ifndef DEBOUNCING_DELAY
#   define DEBOUNCING_DELAY 5
#endif

#if (DEBOUNCING_DELAY > 0)
  static uint16_t debouncing_time;
  static bool debouncing = false;
#endif

#define print_matrix_header()  print("\nr/c 0123456789ABCDEF\n")
#define print_matrix_row(row)  print_bin_reverse16(matrix_get_row(row))
#define matrix_bitpop(i)     bitpop16(matrix[i])
#define ROW_SHIFTER ((uint16_t)1)

/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

static void init_cols(void);
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
    matrix_debouncing[i] = 0;
  }

  matrix_init_quantum();
}

uint8_t matrix_scan(void)
{
  // TODO: everything about scanning
  for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
    select_row(i);
    unselect_rows();
  }

  if (debouncing && (timer_elapsed(debouncing_time) > DEBOUNCING_DELAY)) {
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
      matrix[i] = matrix_debouncing[i];
    }
    debouncing = false;
  }

  matrix_scan_quantum();
  return 1;
}

bool matrix_is_modified(void)
{
#if (DEBOUNCING_DELAY > 0)
  if (debouncing) return false;
#endif
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
 *
 * SLAVE:
 *   row: 0   1   2   3   4
 *   pin:
 *
 * ---------------------------------------- */

static void init_cols(void)
{
  // TODO: set columns to be read
}

/**
 * Set DDR=0 PORT=0 to unselect.
 */
static void unselect_rows(void)
{
  // MASTER  76543210
  DDRB  &= 0b11001111;
  PORTB &= 0b11001111;
  DDRE  &= 0b10111111;
  PORTE &= 0b10111111;
  DDRD  &= 0b01111111;
  PORTD &= 0b01111111;
  DDRC  &= 0b11101111;
  PORTC &= 0b11101111;
}

static void select_row(uint8_t row)
{
  switch (row) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      break;
  }

  // Slave
  switch (row) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    case 4:
      break;
  }
}
