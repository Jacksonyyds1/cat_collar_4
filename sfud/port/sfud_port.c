/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>
#include <string.h>
// #include "sl_udelay.h"
// #include "em_gpio.h"
// #include "em_usart.h"
// #include "spidrv.h"
// #include "sl_spidrv_eusart_exp_config.h"
// #include "sl_spidrv_instances.h"
#include "spi_flash.h"
// #include "em_cmu.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os2.h"
#include "app_log.h"
#include "sl_constants.h"

// #define USE_DYNAMIC_SPI_BUFFER
#define USE_STATIC_SPI_BUFFER

static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);

static SemaphoreHandle_t xMutex_sfud_spi;
// static spi_user_data_t spi_dev = NULL;

extern bool gspi_transfer_complete;
/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
        size_t read_size) {
    
    UNUSED_PARAMETER(spi);

    sfud_err result = SFUD_SUCCESS;
    sl_status_t status;
    uint32_t wait_count = 0;
    const uint32_t timeout = 1000000; // 1s超时

    /**
     * add your spi write and read code
     */

    size_t transfer_total_size = write_size + read_size;

    // app_log_debug("write size=%d, read size=%d, total size=%d\r\n",
    //               write_size, read_size, transfer_total_size);

#ifdef USE_DYNAMIC_SPI_BUFFER
    // 创建临时接收缓冲区
    uint8_t *rx_buffer = malloc(transfer_total_size);
    if (!rx_buffer) {
        app_log_error("malloc rx buffer failed!\r\n");
        free(rx_buffer);
        return SFUD_ERR_NOT_FOUND;
    }

    // 启动传输
    gspi_transfer_complete = false;
    status = sl_si91x_gspi_transfer_data(gspi_driver_handle, write_buf, rx_buffer, transfer_total_size);
    if (status != SL_STATUS_OK) {
        app_log_error("gspi transfer data failed!");
        result = SFUD_ERR_WRITE;
    }
#endif

#ifdef USE_STATIC_SPI_BUFFER

#define GSPI_BUF_SIZE 4200
    // static uint8_t gspi_tx_buf[512];
    static uint8_t gspi_rx_buf[GSPI_BUF_SIZE];
    // 启动传输
    gspi_transfer_complete = false;
    status = sl_si91x_gspi_transfer_data(gspi_driver_handle, write_buf, gspi_rx_buf, transfer_total_size);
    if (status != SL_STATUS_OK) {
        app_log_error("gspi transfer data failed!");
        result = SFUD_ERR_WRITE;
    }
#endif
    // status = sl_si91x_gspi_send_data(gspi_driver_handle, write_buf, write_size);
    // if (status != SL_STATUS_OK) {
    //     app_log_error("gspi send data failed!");
    //     return SFUD_ERR_WRITE;
    // }

    // status = sl_si91x_gspi_receive_data(gspi_driver_handle, rx_buffer, read_size);
    // if (status != SL_STATUS_OK) {
    //     app_log_error("gspi receive data failed!");
    //     return SFUD_ERR_READ;
    // }
//   app_log_info("Free heap size-2: %d bytes", xPortGetFreeHeapSize());

    // 带超时的等待
    while (!gspi_transfer_complete && (wait_count++ < timeout)) {
        taskYIELD();
    }
    if (!gspi_transfer_complete) {
        app_log_error("gspi transfer data timeout!");
        return SFUD_ERR_TIMEOUT;
    }

    // // 不带超时的等待
    // while (!gspi_transfer_complete) {
    //     taskYIELD();
    // }

    // printf("read data: ");
    // for (size_t i = 0; i < read_size; i++){
    //     printf("%02X ", read_buf[i]);
    // }
    // printf("");
#ifdef USE_DYNAMIC_SPI_BUFFER
    // 处理接收数据
    if (read_buf && read_size) {
        memcpy(read_buf, rx_buffer, read_size);
        free(rx_buffer);
    }
#endif

#ifdef USE_STATIC_SPI_BUFFER
    // 处理接收数据
    if (read_buf && read_size) {
        memcpy(read_buf, gspi_rx_buf + write_size, read_size);
    }
#endif

    return (result == SFUD_SUCCESS) ? SFUD_SUCCESS : SFUD_ERR_NOT_FOUND;

    // UNUSED_PARAMETER(spi);
    // sl_status_t status;
    // /**
    //  * add your spi write and read code
    //  */
    // size_t total_size = write_size + read_size;
    // // app_log_debug("write size=%d, read size=%d, total size=%d",
    // //                write_size, read_size, write_size + read_size);
    // // 创建临时发送缓冲区
    // uint8_t *tx_buffer = malloc(total_size);
    // if (!tx_buffer) return SFUD_ERR_NOT_FOUND;
    
    // // 创建临时接收缓冲区
    // uint8_t *rx_buffer = malloc(total_size);
    // if (!rx_buffer) {
    //     free(tx_buffer);
    //     return SFUD_ERR_NOT_FOUND;
    // }
    
    // // 填充发送缓冲区
    // if (write_buf && write_size) {
    //     memcpy(tx_buffer, write_buf, write_size);
    // }

    // // 为读取部分填充dummy字节
    // memset(tx_buffer + write_size, 0xFF, read_size);

    // // 启动传输
    // gspi_transfer_complete = false;
    // status = sl_si91x_gspi_transfer_data(gspi_driver_handle, tx_buffer, rx_buffer, total_size);
    
    // // 等待传输完成
    // while (!gspi_transfer_complete) { 
    //     taskYIELD();
    // }

    // // 处理接收数据
    // if (read_buf && read_size) {
    //     memcpy(read_buf, rx_buffer + write_size, read_size);
    // }
    
    // // printf("read data: ");
    // // for (size_t i = 0; i < read_size; i++){
    // //     printf("%02X ", read_buf[i]);
    // // }
    // // printf("");
    

    // // 清理资源
    // free(tx_buffer);
    // free(rx_buffer);
    
    // return (status == SL_STATUS_OK) ? SFUD_SUCCESS : SFUD_ERR_NOT_FOUND;
}

static void spi_lock(const sfud_spi *spi) {
    UNUSED_PARAMETER(spi);
    xSemaphoreTake(xMutex_sfud_spi, portMAX_DELAY);
}

static void spi_unlock(const sfud_spi *spi) {
  UNUSED_PARAMETER(spi);
  xSemaphoreGive(xMutex_sfud_spi);
}

static void retry_delay(void) {
  for (volatile int i = 0; i < 4000; i++);
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
        uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your port spi bus and device object initialize code like this:
     * 1. rcc initialize
     * 2. gpio initialize
     * 3. spi device initialize
     * 4. flash->spi and flash->retry item initialize
     *    flash->spi.wr = spi_write_read; //Required
     *    flash->spi.qspi_read = qspi_read; //Required when QSPI mode enable
     *    flash->spi.lock = spi_lock;
     *    flash->spi.unlock = spi_unlock;
     *    flash->spi.user_data = &spix;
     *    flash->retry.delay = null;
     *    flash->retry.times = 10000; //Required
     */

    xMutex_sfud_spi = xSemaphoreCreateMutex();
    if (xMutex_sfud_spi == NULL){
        app_log_error("sfud_spi_port_init: create xMutex error");
        while(1);
    }

    switch (flash->index)
    {
        case SFUD_MX25_DEVICE_INDEX: {

            // app_log_info("sfud_spi_port_init");
            /* 同步 Flash 移植所需的接口及数据 */
            flash->spi.wr = spi_write_read;
            flash->spi.lock = spi_lock;
            flash->spi.unlock = spi_unlock;
            // flash->spi.user_data = &spi_dev;
            /* about 100 microsecond delay */
            flash->retry.delay = retry_delay;
            /* adout 60 seconds timeout */
            flash->retry.times = 60 * 10000;
            break;
        }
    }
    // app_log_info("end sfud_spi_port_init");
    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    _app_log_time();
    _app_log_print_color(APP_LOG_LEVEL_DEBUG); 
    _app_log_print_prefix(APP_LOG_LEVEL_DEBUG);
    app_log_append("[SFUD](%s:%ld) ", file, line);
    _app_log_reset_color();
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    app_log_append("%s\r\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    _app_log_time();
    _app_log_print_color(APP_LOG_LEVEL_INFO); 
    _app_log_print_prefix(APP_LOG_LEVEL_INFO);
    app_log_append("[SFUD]");
    _app_log_reset_color();
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    app_log_append("%s\r\n", log_buf);
    va_end(args);
}
