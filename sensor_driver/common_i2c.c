#include "common_i2c.h"
// #include "cmsis_os2.h"
#include "sl_utility.h"
#include "sl_sleeptimer.h"

#include "sl_i2c_instances.h"
#include "sl_si91x_i2c.h"
#include "sl_si91x_peripheral_i2c.h"
#include "app_log.h"


void platform_i2c_init(int i2c_instance)
{
  sl_i2c_status_t i2c_status;
  sl_i2c_config_t sl_i2c_config;

  if (i2c_instance == INSTANCE_ZERO){
    sl_i2c_config = sl_i2c_i2c0_config;
  }else if (i2c_instance == INSTANCE_ONE){
    sl_i2c_config = sl_i2c_i2c1_config;
  }else if (i2c_instance == INSTANCE_TWO){
    sl_i2c_config = sl_i2c_i2c2_config;
  }

  // Initializing I2C instance
  i2c_status = sl_i2c_driver_init(i2c_instance, &sl_i2c_config);
  if (i2c_status != SL_I2C_SUCCESS) {
    app_log_error("sl_i2c_driver_init : Invalid Parameters, Error Code : %u \r\n", i2c_status);
  } else {
    app_log_debug("[i2c%d] Successfully initialized and configured i2c leader\r\n", i2c_instance);
  }
  // Configuring RX and TX FIFO thresholds
  i2c_status = sl_i2c_driver_configure_fifo_threshold(i2c_instance, I2C_TX_FIFO_THRESHOLD, I2C_RX_FIFO_THRESHOLD);
  if (i2c_status != SL_I2C_SUCCESS) {
    app_log_error("sl_i2c_driver_configure_fifo_threshold : Invalid Parameters, Error Code : %u \r\n", i2c_status);
  } else {
    // app_log_debug("[i2c%d]Successfully configured i2c TX & RX FIFO thresholds\r\n", i2c_instance);
  }

}

int32_t platform_write(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint8_t regadr, const uint8_t *buf, uint16_t len)
{
  UNUSED_PARAMETER(handle);
  uint8_t write_buf[len + 1];
  sl_i2c_status_t i2c_status;

  write_buf[0] = regadr;
  memcpy(&write_buf[1], buf, len);

  // app_log_debug("[i2c%d]send data:", i2c_instance);
  // for (int i = 0; i < len+1; i++)
  // {
  //   printf("%02X ", write_buf[i]);
  // }
  // printf("\r\n");
  
  i2c_status = sl_i2c_driver_send_data_blocking(i2c_instance, slave_addr, write_buf, len + 1);
  if (i2c_status != SL_I2C_SUCCESS) {
    app_log_error("sl_i2c_driver_send_data_blocking : Invalid Parameters, Error Code : %u \r\n", i2c_status);
    return SL_I2C_BUS_ERROR;
  }
  return SL_I2C_SUCCESS;
}
int32_t platform_read(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint8_t regadr, uint8_t *buf, uint16_t len)
{
  UNUSED_PARAMETER(handle);
  sl_i2c_status_t i2c_status;
  sl_i2c_transfer_config_t transfer;
  uint8_t write_buf[1] = {regadr};
  
  // 写阶段配置
  transfer.tx_buffer = write_buf;
  transfer.tx_len = 1; // 只写入寄存器地址
  
  // 读阶段配置
  transfer.rx_buffer = buf;
  transfer.rx_len= len; // 读取指定长度的数据
  
  // 执行复合I2C传输
  i2c_status = sl_i2c_driver_transfer_data(i2c_instance, &transfer, slave_addr);

  // app_log_debug("[i2c%d]recieve data:", i2c_instance);
  // for (size_t i = 0; i < len; i++)
  // {
  //   printf("%02X ", buf[i]);
  // }
  // printf("\r\n");
  
  if (i2c_status != SL_I2C_SUCCESS) {
    app_log_error("sl_i2c_driver_receive_data_blocking : Invalid Parameters, Error Code : %u \r\n", i2c_status);
    return SL_I2C_BUS_ERROR;
  }
  return SL_I2C_SUCCESS;
}

int32_t platform_write16(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint16_t value, uint16_t len)
{
    UNUSED_PARAMETER(handle);
    sl_i2c_status_t i2c_status;

    // uint8_t i2c_write_data[2] = {0};  // 增加2字节地址空间
    // uint8_t i2c_value_data[2] = {0};

    // sl_i2c_transfer_config_t transfer;

    // i2c_write_data[0] = (uint8_t)(regadr >> 8);   // 地址高字节
    // i2c_write_data[1] = (uint8_t)(regadr & 0xFF); // 地址低字节
    // i2c_value_data[0] = (uint8_t)(value >> 8);   // 数据高字节
    // i2c_value_data[1] = (uint8_t)(value & 0xFF); // 数据低字节

    // transfer.tx_buffer = i2c_write_data;
    // transfer.tx_len = 2;
    // transfer.rx_buffer = i2c_value_data;
    // transfer.rx_len = len;

    // i2c_status =  sl_i2c_driver_transfer_data(i2c_instance, &transfer, slave_addr);
    // if (i2c_status != SL_I2C_SUCCESS) {
    //   return SL_I2C_BUS_ERROR;
    // }

    uint8_t i2c_write_data[4] = {0};
    i2c_write_data[0] = (uint8_t)(regadr >> 8);
    i2c_write_data[1] = (uint8_t)(regadr & 0xFF);
    i2c_write_data[2] = (uint8_t)(value >> 8);
    i2c_write_data[3] = (uint8_t)(value & 0xFF);


    i2c_status = sl_i2c_driver_send_data_blocking(i2c_instance, slave_addr, i2c_write_data, len);
    if (i2c_status != SL_I2C_SUCCESS) {
        app_log_error("sl_i2c_driver_send_data_blocking failed with %d", i2c_status);
        return SL_I2C_BUS_ERROR;
    }

    // app_log_debug("Sent: [0x%02X][0x%02X][0x%02X][0x%02X", i2c_write_data[0], i2c_write_data[1], i2c_write_data[2], i2c_write_data[3]);

    value = (uint16_t)(i2c_write_data[2] << 8) | i2c_write_data[3];

    return SL_I2C_SUCCESS;
}
int32_t platform_read16(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint16_t *value, uint16_t len)
{
    UNUSED_PARAMETER(handle);
    sl_i2c_status_t i2c_status;
    uint8_t i2c_write_data[2] = {0};
    uint8_t i2c_read_data[2] = {0};
    
    // sl_i2c_transfer_config_t transfer;
    i2c_write_data[0] = (uint8_t)(regadr >> 8);
    i2c_write_data[1] = (uint8_t)(regadr & 0xFF);

    // transfer.tx_buffer = i2c_write_data;
    // transfer.tx_len = 2;
    // transfer.rx_buffer = i2c_read_data;
    // transfer.rx_len = len;
    
    // i2c_status = sl_i2c_driver_transfer_data(i2c_instance, &transfer, slave_addr);
    // if (i2c_status != SL_I2C_SUCCESS) {
    //     return SL_I2C_BUS_ERROR;
    // }

    // Enabling repeated start
    sl_i2c_driver_enable_repeated_start(i2c_instance, true);

    i2c_status = sl_i2c_driver_send_data_blocking(i2c_instance, slave_addr, i2c_write_data, 2);
    if (i2c_status != SL_I2C_SUCCESS) {
        return SL_I2C_BUS_ERROR;
    }
    
    // Disabling repeated start
    sl_i2c_driver_enable_repeated_start(i2c_instance, false);

    i2c_status = sl_i2c_driver_receive_data_blocking(i2c_instance, slave_addr, i2c_read_data, len);
    if (i2c_status != SL_I2C_SUCCESS) {
        return SL_I2C_BUS_ERROR;
    }

    // app_log_debug("Received: [0x%02X][0x%02X]", 
    //                i2c_read_data[0], i2c_read_data[1]);
    
    *value = (uint16_t)(i2c_read_data[0] << 8) | i2c_read_data[1];
    

    return SL_I2C_SUCCESS;
}

int32_t platform_read32(sl_i2c_instance_t i2c_instance, int slave_addr, void *handle, uint16_t regadr, uint32_t *value, uint16_t len)
{
    UNUSED_PARAMETER(handle);
    sl_i2c_status_t i2c_status;

    uint8_t i2c_write_data[2] = {0};
    uint8_t i2c_read_data[4] = {0};

    i2c_write_data[0] = (uint8_t)(regadr >> 8);
    i2c_write_data[1] = (uint8_t)(regadr & 0xFF);

    // Enabling repeated start
    sl_i2c_driver_enable_repeated_start(i2c_instance, true);
    
    i2c_status = sl_i2c_driver_send_data_blocking(i2c_instance, slave_addr, i2c_write_data, 2);
    if (i2c_status != SL_I2C_SUCCESS) {
      return SL_I2C_BUS_ERROR;
    }
    
    // Disabling repeated start
    sl_i2c_driver_enable_repeated_start(i2c_instance, false);
    
    i2c_status = sl_i2c_driver_receive_data_blocking(i2c_instance, slave_addr, i2c_read_data, len);
    if (i2c_status != SL_I2C_SUCCESS) {
      return SL_I2C_BUS_ERROR;
    }
    // app_log_debug("Received: [0x%02X][0x%02X][0x%02X][0x%02X]", i2c_read_data[0], i2c_read_data[1], i2c_read_data[2], i2c_read_data[3]);
    
    *value = (i2c_read_data[0] << 8) | i2c_read_data[1] | (i2c_read_data[2] << 24) | (i2c_read_data[3] << 16);
    
    return SL_I2C_SUCCESS;
}

void platform_delay(uint32_t ms)
{
    sl_sleeptimer_delay_millisecond(ms);
}
