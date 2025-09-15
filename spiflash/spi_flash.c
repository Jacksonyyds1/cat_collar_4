#include "sfud.h"
#include "app_log.h"
#include "spi_flash.h"
#include <string.h>

#include "common.h"

#include "cmsis_os2.h"
#include "sl_status.h"
#include "sl_si91x_gspi.h"
#include "sl_si91x_gspi_common_config.h"

sl_gspi_handle_t gspi_driver_handle = NULL;

#define GSPI_BITRATE                 20000000  // Bitrate for setting the clock division factor
#define GSPI_BIT_WIDTH               8         // Default Bit width
#define GSPI_SWAP_READ_DATA          1         // true to enable and false to disable swap read
#define GSPI_SWAP_WRITE_DATA         0         // true to enable and false to disable swap write

// #define GSPI_BUFFER_SIZE             1024      // Size of buffer
// static uint8_t gspi_data_in[GSPI_BUFFER_SIZE];
// static uint8_t gspi_data_out[GSPI_BUFFER_SIZE];

// GSPI传输完成回调
volatile bool gspi_transfer_complete = false;

void gspi_transfer_callback(uint32_t event) {
    if (event == SL_GSPI_TRANSFER_COMPLETE) {
        gspi_transfer_complete = true;
    }
}

static uint8_t sSpiFlashInit_OK = 0;
void spi_flash_init(void)
{
    sl_status_t status;
    sl_gspi_version_t version;
//    sl_gspi_status_t gspi_status;
    sl_gspi_control_config_t config;

    config.bit_width         = GSPI_BIT_WIDTH;
    config.bitrate           = GSPI_BITRATE;
    config.clock_mode        = SL_GSPI_MODE_0;
    config.slave_select_mode = SL_GSPI_MASTER_HW_OUTPUT;
    config.swap_read         = GSPI_SWAP_READ_DATA;
    config.swap_write        = GSPI_SWAP_WRITE_DATA;

    version = sl_si91x_gspi_get_version();
    app_log_info("GSPI version is fetched successfully \n");
    app_log_info("API version is %d.%d.%d\n", version.release, version.major, version.minor);

    status = sl_si91x_gspi_init(SL_GSPI_MASTER, &gspi_driver_handle);
    if (status != SL_STATUS_OK) {
        app_log_error("sl_si91x_gspi_init: Error Code : %lu \n", status);
    }else{
        app_log_debug("GSPI initialization is successful \n");
        // Fetching the status of GSPI i.e., busy, data lost and mode fault
        // gspi_status = sl_si91x_gspi_get_status(gspi_driver_handle);
        // app_log_debug("GSPI status is fetched successfully \r\n");
        // app_log_debug("Busy: %d\n", gspi_status.busy);
        // app_log_debug("Data_Lost: %d\n", gspi_status.data_lost);
        // app_log_debug("Mode_Fault: %d\r\n", gspi_status.mode_fault);
    }

    status = sl_si91x_gspi_set_configuration(gspi_driver_handle, &config);
    if (status != SL_STATUS_OK) {
      app_log_debug("sl_si91x_gspi_control: Error Code : %lu \r\n", status);
    }

    // 注册回调函数
    status = sl_si91x_gspi_register_event_callback(gspi_driver_handle, gspi_transfer_callback);
    if (status != SL_STATUS_OK) {
        app_log_error("sl_si91x_gspi_register_event_callback: Error Code : %lu \r\n", status);
    }else{
        app_log_debug("GSPI event callback registered successfully \r\n");
    }
    
    // 设置从设备号
    sl_si91x_gspi_set_slave_number(GSPI_SLAVE_0);

    // for (uint16_t i = 0; i < 1024; i++) {
    //   gspi_data_out[i] = (uint8_t)(i + 1);
    // }

    // // 启动传输
    // gspi_transfer_complete = false;
    // status = sl_si91x_gspi_transfer_data(gspi_driver_handle, gspi_data_out, gspi_data_in, sizeof(gspi_data_out));
    // if (status != SL_STATUS_OK) {
    //     app_log_error("gspi transfer data failed!\r\n");
    // }

    // // 等待传输完成
    // while (!gspi_transfer_complete) {
    //     taskYIELD();
    // }

    if (sfud_init() == SFUD_SUCCESS) {
        app_log_info("sfud_init success.\r\n");
        setSystemStatus(SST_SPIFLASH, SYSSTATUS_OK);
        sSpiFlashInit_OK = 1;
    }
    else
        setSystemStatus(SST_SPIFLASH, SYSSTATUS_SPIFLASH_ERROR);
}

// void spiFlash_power_onoff(uint8_t onoff)
// {
//     if(onoff){
//         GPIO_PinOutSet(SPI_FLASH_POWER_EN_PORT, SPI_FLASH_POWER_EN_PIN);
//     }else{
//         GPIO_PinOutClear(SPI_FLASH_POWER_EN_PORT, SPI_FLASH_POWER_EN_PIN);
//     }
// }

// void spiFlash_shutdown(void)
// {
//   GPIO_PinOutClear(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN);
//   sl_udelay_wait(20);
//   EUSART_Spi_TxRx(sl_spidrv_eusart_exp_handle->peripheral.eusartPort, 0xB9);
//   sl_udelay_wait(20);
//   GPIO_PinOutSet(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN);
// }

#if SPI_FLASH_TYPE == SPI_FLASH_TYPE_GD25
void spiFlash_release_shutdown(void)
{
// #if 1
//   GPIO_PinOutClear(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN);
//   sl_udelay_wait(20);
//   EUSART_Spi_TxRx(sl_spidrv_eusart_exp_handle->peripheral.eusartPort, 0xAB);
//   sl_udelay_wait(20);
//   GPIO_PinOutSet(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN);
//   sl_udelay_wait(200);
// #else
//   const sfud_flash *flash = sfud_get_device_table () + 0;
//   return sfud_reset(flash);
// #endif
}
#endif

uint8_t spi_flash_erase(uint32_t flashAddr, size_t len)
{
    uint8_t ret = 0;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    if(!sSpiFlashInit_OK) return SFUD_ERR_NOT_FOUND;

    ret = sfud_erase(flash, flashAddr, len);
    if(ret == SFUD_SUCCESS)
      setSystemStatus(SST_SPIFLASH, SYSSTATUS_OK);
    else
      setSystemStatus(SST_SPIFLASH, SYSSTATUS_SPIFLASH_ERROR);
    return ret;
}

uint8_t spi_flash_write(uint32_t flashAddr, size_t len, uint8_t * pbuf)
{
    uint8_t ret;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    if(!sSpiFlashInit_OK) return SFUD_ERR_NOT_FOUND;

    ret = sfud_write(flash, flashAddr, len, pbuf);
    if(ret == SFUD_SUCCESS)
       setSystemStatus(SST_SPIFLASH, SYSSTATUS_OK);
    else
       setSystemStatus(SST_SPIFLASH, SYSSTATUS_SPIFLASH_ERROR);
    return ret;
}

uint8_t spi_flash_read(uint32_t flashAddr, size_t len, uint8_t * pbuf)
{
    uint8_t ret;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    if(!sSpiFlashInit_OK) return SFUD_ERR_NOT_FOUND;
    ret = sfud_read(flash, flashAddr, len, pbuf);
    if(ret == SFUD_SUCCESS)
       setSystemStatus(SST_SPIFLASH, SYSSTATUS_OK);
    else
       setSystemStatus(SST_SPIFLASH, SYSSTATUS_SPIFLASH_ERROR);

    return ret;
}

uint8_t spi_flash_format(void)
{
    uint8_t ret;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    if(!sSpiFlashInit_OK) 
        return SFUD_ERR_NOT_FOUND;

    ret = sfud_erase(flash, 0, flash->chip.capacity);
    if(ret == SFUD_SUCCESS)
       setSystemStatus(SST_SPIFLASH, SYSSTATUS_OK);
    app_log_debug("SPI Flash is formatted!\r\n");
    return SFUD_SUCCESS;
}

void spi_flash_test(void)
{
    const uint32_t test_base_addr = 0x1000; // 测试基地址(避开0地址)
    const uint16_t test_data_size = 512;    // 测试数据大小
    uint8_t write_buf[test_data_size];
    uint8_t read_buf[test_data_size];
    sfud_err result = SFUD_SUCCESS;

    const sfud_flash *flash = sfud_get_device_table() + 0;

    // 1. 准备测试数据
    for (int i = 0; i < test_data_size; i++) {
        write_buf[i] = (i % 256);
        if (i % 32 == 0) write_buf[i] = 0xAA;   // 每32字节插入标志
        if (i % 64 == 0) write_buf[i] = 0x55;   // 每64字节插入标志
        if (i == 0) write_buf[i] = 0x5A;         // 起始标志
        if (i == test_data_size-1) write_buf[i] = 0xA5; // 结束标志
    }

    app_log_debug("=== Flash test Start ===\r\n");
    
    // 2. 擦除测试区域 (1KB对齐)
    result = sfud_erase(flash, test_base_addr, 1 * 1024);
    app_log_debug("erase result: %s (0x%X)\r\n", 
            result == SFUD_SUCCESS ? "success" : "failure", result);
    
    // 3. 写入测试数据
    result = sfud_write(flash, test_base_addr, test_data_size, write_buf);
    app_log_debug("write result: %s (0x%X)\r\n", 
            result == SFUD_SUCCESS ? "success" : "failure", result);
    
    // 4. 读取验证
    memset(read_buf, 0, test_data_size);
    result = sfud_read(flash, test_base_addr, test_data_size, read_buf);
    app_log_debug("read result: %s (0x%X)\r\n", 
            result == SFUD_SUCCESS ? "success" : "failure", result);
    
    // 5. 数据校验
    int error_count = 0;
    for (int i = 0; i < test_data_size; i++) {
        if (read_buf[i] != write_buf[i]) {
            error_count++;
            if (error_count < 10) {  // 只打印前10个错误
                app_log("data mismatch @ 0x%04X: write=0x%02X, read=0x%02X\r\n", 
                        test_base_addr + i, write_buf[i], read_buf[i]);
            }
        }
    }
    
    // 6. 打印关键位置验证
    app_log_debug("--- key position validation ---\r\n");
    app_log_debug("start byte (0x%X): write=0x%02X, read=0x%02X %s\r\n", 
            test_base_addr, write_buf[0], read_buf[0],
            write_buf[0] == read_buf[0] ? "✓" : "✗");
    
    app_log_debug("32byte -> (0x%X): write=0x%02X, read=0x%02X %s\r\n", 
            test_base_addr+32, write_buf[32], read_buf[32],
            write_buf[32] == read_buf[32] ? "✓" : "✗");
    
    app_log_debug("64byte -> (0x%X): write=0x%02X, read=0x%02X %s\r\n", 
            test_base_addr+64, write_buf[64], read_buf[64],
            write_buf[64] == read_buf[64] ? "✓" : "✗");
    
    app_log_debug("end byte (0x%X): write=0x%02X, read=0x%02X %s\r\n", 
            test_base_addr+test_data_size-1, 
            write_buf[test_data_size-1], read_buf[test_data_size-1],
            write_buf[test_data_size-1] == read_buf[test_data_size-1] ? "✓" : "✗");
    
    // 7. 测试结论
    app_log_debug("--------------------------------\r\n");
    app_log_debug("test data size: %d byte\r\n", test_data_size);
    app_log_debug("error count: %d\r\n", error_count);
    app_log_debug("test result: %s\r\n", error_count == 0 ? "pass" : "fail");
    app_log_debug("================================\r\n");

    // // 0. 打印Flash信息
    // uint16_t i = 0;
    // const sfud_flash *flash = sfud_get_device_table() + 0;
    //     app_log_debug("\n=== Flash Basic Test ===\n");
    
    // // 1. 测试地址
    // const uint32_t TEST_ADDR = 0x0100;
    // const int TEST_SIZE = 10;
    
    // // 2. 准备测试数据
    // uint8_t write_data[TEST_SIZE];
    // uint8_t read_data[TEST_SIZE];
    
    // for (int i = 0; i < TEST_SIZE; i++) {
    //     // write_data[i] = i % 256; // 0-255循环
    //     write_data[i] = 0x1A;
    // }
    
    // // 3. 擦除区域
    // app_log_debug("Erasing...\r\n");
    // sfud_erase(flash, TEST_ADDR, 1024);
    // app_log_debug("OK\n");

    // // 4.0 写入数据
    // app_log_debug("Writing.0..\r\n");
    // sfud_err result = sfud_write(flash, TEST_ADDR, 1, write_data);
    // if (result != SFUD_SUCCESS) {
    //     app_log_debug("FAILED (0x%X)\r\n", result);
    //     return;
    // }
    // app_log_debug("OK.0\r\n");

    // // 4.1 写入数据
    // app_log_debug("Writing.1..\r\n");
    // result = sfud_write(flash, TEST_ADDR, TEST_SIZE, write_data);
    // if (result != SFUD_SUCCESS) {
    //     app_log_debug("FAILED (0x%X)\r\n", result);
    //     return;
    // }
    // app_log_debug("OK.1\r\n");
    
    // // // 4.2 写入数据
    // // app_log_debug("Writing.2..\r\n");
    // // result = sfud_write(flash, TEST_ADDR, TEST_SIZE, write_data);
    // // if (result != SFUD_SUCCESS) {
    // //     app_log_debug("FAILED (0x%X)\r\n", result);
    // //     return;
    // // }
    // // app_log_debug("OK.2\r\n");
    
    // // 5. 读取验证
    // app_log_debug("Reading...\r\n");
    // result = sfud_read(flash, TEST_ADDR, TEST_SIZE, read_data);
    // if (result != SFUD_SUCCESS) {
    //     app_log_debug("FAILED (0x%X)\r\n", result);
    //     return;
    // }
    // app_log_debug("OK\r\n");
    
    // printf ("sfud_read result=0x%x\r\n", result);
    // for (i = 0; i < sizeof(read_data); i++)
    // {
    //     printf ("%d:%02x ", i, read_data[i]);
    //     if ((i + 1) % 16 == 0){
    //         printf ("\r\n");
    //     }
    // }

    // // 6. 简单验证
    // int errors = 0;
    // for (int i = 0; i < TEST_SIZE; i++) {
    //     if (read_data[i] != write_data[i]) {
    //         errors++;
    //     }
    // }
    
    // // 7. 打印关键位置
    // app_log_debug("Start: W=0x%02X, R=0x%02X %s\n", 
    //        write_data[0], read_data[0],
    //        write_data[0] == read_data[0] ? "✓" : "✗");
    
    // app_log_debug("Middle: W=0x%02X, R=0x%02X %s\n", 
    //        write_data[TEST_SIZE/2], read_data[TEST_SIZE/2],
    //        write_data[TEST_SIZE/2] == read_data[TEST_SIZE/2] ? "✓" : "✗");
    
    // app_log_debug("End: W=0x%02X, R=0x%02X %s\n", 
    //        write_data[TEST_SIZE-1], read_data[TEST_SIZE-1],
    //        write_data[TEST_SIZE-1] == read_data[TEST_SIZE-1] ? "✓" : "✗");
    
    // app_log_debug("Total errors: %d\n", errors);
    // app_log_debug("Test Result: %s\n", errors == 0 ? "PASSED" : "FAILED");
    // app_log_debug("==============================\n");
}

