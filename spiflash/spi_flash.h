/*
 * spiFlash.h
 *
 *  Created on: 2025年7月23日
 *      Author: YQ05165
 */

#ifndef SPI_FLASH_H_
#define SPI_FLASH_H_

#include "sl_si91x_gspi.h"

#define SPI_FLASH_TYPE_MX25   0
#define SPI_FLASH_TYPE_GD25   1
#define SPI_FLASH_TYPE        SPI_FLASH_TYPE_GD25

#define SPIFLASH_PAGE_SIZE    256           //页大小
#define SPIFLASH_SECTOR       (4*1024)      //扇区大小

#define SENSORDATA_FLASH_BASE_ADDR       0
#define SENSORDATA_FLASH_SIZE            (1024*1024*3)

extern sl_gspi_handle_t gspi_driver_handle;

void spi_flash_init(void);
// void spiFlash_shutdown(void);
#if SPI_FLASH_TYPE == SPI_FLASH_TYPE_GD25
void spiFlash_release_shutdown(void);
#endif
// void spiFlash_power_onoff(uint8_t onoff);
uint8_t spi_flash_format(void);
uint8_t spi_flash_erase(uint32_t flashAddr, size_t len);
uint8_t spi_flash_write(uint32_t flashAddr, size_t len, uint8_t * pbuf);
uint8_t spi_flash_read(uint32_t flashAddr, size_t len, uint8_t * pbuf);
void spi_flash_test(void);

#endif /* SPIFLASH_H_ */
