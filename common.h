/*
 * common.h
 *
 *  Created on: 2025年7月18日
 *      Author: YQ05165
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "sl_constants.h"
#include "app_log.h"

#define CATCOLLAR_APPLICATION_MAJOR_VERSION   1
#define CATCOLLAR_APPLICATION_MINOR_VERSION   0
#define CATCOLLAR_APPLICATION_PATCH_VERSION   2
#define CATCOLLAR_FW_VERSION  CATCOLLAR_APPLICATION_MAJOR_VERSION * 100 + CATCOLLAR_APPLICATION_MINOR_VERSION * 10 + CATCOLLAR_APPLICATION_PATCH_VERSION

// FIXME: find correct values
#define DEVICE_MODEL 'a'
#define MFG_CODE     '1'
#define FACTORY_CODE '2'
#define PCBA_MAJOR   (0)
#define PCBA_MINOR   (0)
#define PCBA_PATCH   (1)


#define SYS_CONFIG_MAGIC (('U' << 0)|('C' << 8)|('M' << 16)|('A' << 24))
typedef struct
{
	unsigned int magic;
  unsigned char auto_shutdown_timer;
	int device_upgrade_flag;
	int fw_version;
}sys_config_info_t;
#define AUTO_SHUTDOWN_TIMER_DEFAULT  10

#define ENABLE_WATCHDOG 0
#define ENABLE_BLE_AUTH 0           // BLE连接后是否需要鉴权，前期关闭方便测试
#define ENABLE_BLE_CRCCHECK  1      // BLE收到的数据包是否进行CRC校验，前期关闭方便测试
#define ENABLE_BLE_SYS_PAIR_BOND 0  // 开启后移动端在连接建立后弹出系统提示框进行绑定

#define BLE_DEVICE_NAME_LEN   13
#define CRC8(buf, len)  crc8Ex(buf, len, 0xff, 0x07)

#define SYSINFO_PREFLAG 0xEAE55AA5
typedef struct _sysInfo{
  char     deviceAddr[18];  //设备地址
  char     bondUserID[32];  //设备绑定的用户
  char     authSecret[32];  //鉴权密钥
  char     wifiSSID[30];    //WIFI SSID
  char     wifiPWD[20];     //WIFI PASSWORD
  bool     wifiSetFlag;     //WIFI设置标志
}SysInfo;

typedef struct _sysStatus{
  uint8_t  imu_status;      // 惯性传感器状态
  uint8_t  airPress_status; // 气压计状态
  uint8_t  spiFlash_status; // SPI FLASH状态
  uint8_t  wifi_status;     // wifi 模块状态
}SysStatus;

typedef enum {
  SST_IMU = 0,
  SST_AIRPRESS,
  SST_SPIFLASH,
  SST_WIFI,
  SST_UNKNOWN,
}SysStatusType;

typedef enum{
  SYSSTATUS_OK = 0,
  SYSSTATUS_IMU_ERROR,
  SYSSTATUS_AIRPRESS_ERROR,
  SYSSTATUS_SPIFLASH_ERROR,
  SYSSTATUS_WIFI_ERROR,
  SST_UNKNOWN_ERROR,
}SysStatusValue;

typedef enum {
    INTRE_SUCCESS = 0,                                      /*成功 */
    INTRE_ERROR = 1,                                        /*通用错误 */
} intre_err;

typedef enum {
  GEOFENCES_STATUS_OFF = 0,
  GEOFENCES_STATUS_ON = 1,
  GEOFENCES_STATUS_INVALID = 0xff,
} geofences_status;

SysStatus* getSystemStatus(void);
void setSystemStatus(SysStatusType type, SysStatusValue status);
const char* get_app_version(void);

void unix_timestamp_init(void);
void set_unix_timestamp(uint32_t timestamp);
uint32_t get_unix_timestamp(void);
uint64_t get_unix_timestamp_ms(void);
int IsSetUnixTimeStamp(void);
const char* get_unix_timestamp_string(void);

// 16位转换函数
uint16_t sys_cpu_to_be16(uint16_t val);
uint16_t sys_be16_to_cpu(uint16_t val);
// 32位转换函数
uint32_t sys_cpu_to_be32(uint32_t val);
uint32_t sys_be32_to_cpu(uint32_t val);
// 64位转换函数
uint64_t sys_cpu_to_be64(uint64_t val);
uint64_t sys_be64_to_cpu(uint64_t val);

uint16_t utils_crc16_modbus(const uint8_t *data, uint16_t length);
uint16_t test_utils_crc16_modbus(const uint8_t *data, uint16_t length);

char* base64_encode(const uint8_t *data, size_t input_length, size_t *output_length);

#endif /* COMMON_H_ */