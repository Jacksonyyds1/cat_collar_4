/*******************************************************************************
* @file  wifi_ota_config.h
* @brief WiFi OTA升级配置文件
*******************************************************************************
* # License
* <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

#ifndef WIFI_OTA_CONFIG_H
#define WIFI_OTA_CONFIG_H

#include <stdint.h>

// Macro to set specified bit position
#ifndef BIT
#define BIT(a) ((uint32_t)1U << a)
#endif

/*==============================================*/
/**
 * OTA Configuration
 */

// 固件更新类型
#define M4_FW_UPDATE       0
#define TA_FW_UPDATE       1
#define COMBINED_FW_UPDATE 2

// 设置固件更新类型 (默认为TA固件更新)
#define FW_UPDATE_TYPE TA_FW_UPDATE

// 是否加载证书到设备flash
#define LOAD_CERTIFICATE 1

// 启用HTTPS支持
#define HTTPS_SUPPORT BIT(0)

// 启用IPv6 (默认使用IPv4)
#define HTTPV6 BIT(3)

// 启用HTTP POST大数据特性
#define HTTP_POST_DATA BIT(5)

// 使用HTTP版本1.1
#define HTTP_V_1_1 BIT(6)

// 启用用户定义的HTTP内容类型
#define HTTP_USER_DEFINED_CONTENT_TYPE BIT(7)

// 设置证书索引 (0, 1, 2)
#define CERTIFICATE_INDEX 0

/*==============================================*/
/**
 * AWS S3配置
 */

// AWS S3存储桶配置
#define AWS_S3_BUCKET_HOST    "cat-firmware-bucket-001.s3.us-east-2.amazonaws.com"
#define AWS_S3_REGION         "us-east-2"

// 固件文件配置
#define FIRMWARE_VERSION_FILE "firmware/version.txt"
#define FIRMWARE_BINARY_FILE  "firmware/yq-catcollar-mainboard.bin"

// HTTP配置
#define OTA_FLAGS       HTTPS_SUPPORT
#define OTA_HTTP_PORT   443
#define OTA_TIMEOUT     600000
#define DNS_TIMEOUT     20000
#define MAX_DNS_RETRY_COUNT 5

// 版本检查间隔 (秒)
#define VERSION_CHECK_INTERVAL 3600  // 1小时检查一次

// 使用SDK提供的AWS证书
#define USE_SDK_AWS_CERTIFICATE 1

/*==============================================*/
/**
 * 版本管理
 */

// 当前固件版本 (在构建时定义)
#ifndef CURRENT_FIRMWARE_VERSION
#define CURRENT_FIRMWARE_VERSION "1.0.0"
#endif

// 版本字符串最大长度
#define MAX_VERSION_STRING_LENGTH 32

/*==============================================*/
/**
 * HTTP扩展头
 */

// 用于AWS S3访问的HTTP扩展头 (如果需要认证)
#define HTTP_EXTENDED_HEADER NULL

// 用户名和密码 (对于S3公共访问可以为空)
#define OTA_USERNAME ""
#define OTA_PASSWORD ""

/*==============================================*/
/**
 * 调试配置
 */

// 启用OTA调试打印
#define OTA_DEBUG_ENABLE 1

#if OTA_DEBUG_ENABLE
#define OTA_LOG_INFO(...)  printf("[OTA INFO] " __VA_ARGS__)
#define OTA_LOG_ERROR(...) printf("[OTA ERROR] " __VA_ARGS__)
#define OTA_LOG_DEBUG(...) printf("[OTA DEBUG] " __VA_ARGS__)
#else
#define OTA_LOG_INFO(...)
#define OTA_LOG_ERROR(...)
#define OTA_LOG_DEBUG(...)
#endif

/*==============================================*/
/**
 * OTA状态定义
 */

typedef enum {
  OTA_STATE_IDLE = 0,
  OTA_STATE_CHECKING_VERSION,
  OTA_STATE_DOWNLOADING,
  OTA_STATE_INSTALLING,
  OTA_STATE_COMPLETE,
  OTA_STATE_ERROR
} ota_state_t;

typedef enum {
  OTA_ERROR_NONE = 0,
  OTA_ERROR_NETWORK,
  OTA_ERROR_DNS_RESOLVE,
  OTA_ERROR_HTTP_REQUEST,
  OTA_ERROR_VERSION_PARSE,
  OTA_ERROR_DOWNLOAD_FAILED,
  OTA_ERROR_INSTALL_FAILED,
  OTA_ERROR_TIMEOUT
} ota_error_t;

#endif // WIFI_OTA_CONFIG_H