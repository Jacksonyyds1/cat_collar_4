#ifndef _COMMON_I2C_H_
#define _COMMON_I2C_H_

#include "common.h"
#include "cmsis_os2.h"
#include "sl_utility.h"
#include "sl_si91x_i2c.h"

#define INSTANCE_ZERO 0 // For instance 0
#define INSTANCE_ONE  1 // For instance 1
#define INSTANCE_TWO  2 // For ulp instance (instance 2)
#define I2C_TX_FIFO_THRESHOLD    0     // FIFO threshold
#define I2C_RX_FIFO_THRESHOLD    0     // FIFO threshold

void platform_i2c_init(int i2c_instance);
int32_t platform_write(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint8_t regadr, const uint8_t *buf, uint16_t len);
int32_t platform_read(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint8_t regadr, uint8_t *buf, uint16_t len);
int32_t platform_write16(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint16_t buf, uint16_t len);
int32_t platform_read16(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint16_t *buf, uint16_t len);
int32_t platform_read32(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint32_t *buf, uint16_t len);
void platform_delay(uint32_t ms);

#endif /* _COMMON_I2C_H_ */