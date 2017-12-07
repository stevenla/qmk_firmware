/**
 * High-level API for I2C with retries
 */

#ifndef SPLIT_HELPER_H
#define SPLIT_HELPER_H

#include "i2c.h"

#define return_if_status(status) do { if (status > 0) { return status; } } while (0)

typedef uint8_t status_t;
typedef status_t (*init_func_t)(void);

typedef struct SplitStruct {
  uint8_t is_connected;
  init_func_t init_func;
} Split;

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

status_t raw_write_byte(uint8_t addr, uint8_t byte);
status_t raw_write_word(uint8_t addr, uint8_t byteA, uint8_t byteB);
status_t raw_read_byte(uint8_t addr, uint8_t* byte);
status_t raw_read_word(uint8_t addr, uint8_t* byteA, uint8_t* byteB);

void init_master(Split* split, init_func_t init_func);
void write_byte(Split* split, uint8_t addr, uint8_t byte);
void write_word(Split* split, uint8_t addr, uint8_t byteA, uint8_t byteB);
uint8_t read_byte(Split* split, uint8_t addr, uint8_t def);
uint16_t read_word(Split* split, uint8_t addr, uint16_t def);

#endif
