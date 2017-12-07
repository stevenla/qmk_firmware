#include "split_helper.h"

status_t raw_write_byte(uint8_t addr, uint8_t byte) {
  return_if_status(i2c_master_start(TWI_ADDR_WRITE));
  return_if_status(i2c_master_write(addr));
  return_if_status(i2c_master_write(byte));
  i2c_master_stop();
  return 0;
}

status_t raw_write_word(uint8_t addr, uint8_t byteA, uint8_t byteB) {
  return_if_status(i2c_master_start(TWI_ADDR_WRITE));
  return_if_status(i2c_master_write(addr));
  return_if_status(i2c_master_write(byteA));
  return_if_status(i2c_master_write(byteB));
  i2c_master_stop();
  return 0;
}

status_t raw_read_byte(uint8_t addr, uint8_t* byte) {
  return_if_status(i2c_master_start(TWI_ADDR_WRITE));
  return_if_status(i2c_master_write(addr));
  return_if_status(i2c_master_start(TWI_ADDR_READ));
  *byte = i2c_master_read(1);
  i2c_master_stop();
  return 0;
}

status_t raw_read_word(uint8_t addr, uint8_t* byteA, uint8_t* byteB) {
  return_if_status(i2c_master_start(TWI_ADDR_WRITE));
  return_if_status(i2c_master_write(addr));
  return_if_status(i2c_master_start(TWI_ADDR_READ));
  *byteA = i2c_master_read(1);
  *byteB = i2c_master_read(1);
  i2c_master_stop();
  return 0;
}

status_t _init(Split* split) {
  uint8_t is_connected = (*split).is_connected;
  if (is_connected) {
    println("already connected");
    return 0;
  }

  init_func_t init_func = (*split).init_func;
  status_t status = (*init_func)();
  if (status == 0) {
    println("successful connect")
    (*split).is_connected = 1;
    return 0;
  } else {
    println("unsuccessful");
    return status;
  }
}

void _disconnect(Split* split) {
  (*split).is_connected = 0;
  println("disconnected")
}

void init_master(Split* split, init_func_t init_func) {
  (*split).init_func = init_func;
  (*split).is_connected = 0;
  i2c_master_init();
  _init(split);
}

void write_byte(Split* split, uint8_t addr, uint8_t byte) {
  if (
    _init(split) ||
    raw_write_byte(addr, byte)
  ) {
    _disconnect(split);
  }
}

void write_word(Split* split, uint8_t addr, uint8_t byteA, uint8_t byteB) {
  if (
    _init(split) ||
    raw_write_word(addr, byteA, byteB)
  ) {
    _disconnect(split);
  }
}

uint8_t read_byte(Split* split, uint8_t addr, uint8_t def) {
  uint8_t byte;
  if (
    _init(split) ||
    raw_read_byte(addr, &byte)
  ) {
    _disconnect(split);
    return def;
  } else {
    return byte;
  }
}

uint16_t read_word(Split* split, uint8_t addr, uint16_t def) {
  uint8_t byteA;
  uint8_t byteB;
  if (
    _init(split) ||
    raw_read_word(addr, &byteA, &byteB)
  ) {
    _disconnect(split);
    return def;
  } else {
    return (byteB << 8) | byteA;
  }
}
