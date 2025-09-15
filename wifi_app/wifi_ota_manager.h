/*******************************************************************************
* @file  wifi_ota_manager.h
* @brief WiFi OTA升级管理器头文件
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

#ifndef WIFI_OTA_MANAGER_H
#define WIFI_OTA_MANAGER_H

#include "sl_status.h"
#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"
#include "wifi_ota_config.h"
#include "cmsis_os2.h"
#include <stdbool.h>
#include <stdint.h>

/*==============================================*/
/**
 * OTA管理器结构体
 */

typedef struct {
  ota_state_t current_state;
  ota_error_t last_error;
  char current_version[MAX_VERSION_STRING_LENGTH];
  char latest_version[MAX_VERSION_STRING_LENGTH];
  uint32_t last_check_time;
  uint32_t download_progress;
  uint32_t total_size;
  bool auto_check_enabled;
  bool update_available;
} ota_manager_t;

/*==============================================*/
/**
 * OTA回调函数类型
 */

typedef void (*ota_progress_callback_t)(uint32_t progress, uint32_t total);
typedef void (*ota_state_callback_t)(ota_state_t state, ota_error_t error);

/*==============================================*/
/**
 * 版本比较结果
 */

typedef enum {
  VERSION_OLDER = -1,
  VERSION_SAME = 0,
  VERSION_NEWER = 1,
  VERSION_INVALID = -2
} version_compare_result_t;

/*==============================================*/
/**
 * 函数声明
 */

/**
 * @brief 初始化OTA管理器
 * @return sl_status_t 状态码
 */
sl_status_t ota_manager_init(void);

/**
 * @brief 启动OTA任务
 * @return sl_status_t 状态码
 */
sl_status_t ota_manager_start_task(void);

/**
 * @brief 检查固件版本更新
 * @return sl_status_t 状态码
 */
sl_status_t ota_check_for_updates(void);

/**
 * @brief 开始固件下载和更新
 * @return sl_status_t 状态码
 */
sl_status_t ota_start_update(void);

/**
 * @brief 获取当前OTA状态
 * @return ota_state_t 当前状态
 */
ota_state_t ota_get_current_state(void);

/**
 * @brief 获取最后一次错误
 * @return ota_error_t 错误码
 */
ota_error_t ota_get_last_error(void);

/**
 * @brief 获取当前固件版本
 * @return const char* 版本字符串
 */
const char* ota_get_current_version(void);

/**
 * @brief 获取最新可用版本
 * @return const char* 版本字符串
 */
const char* ota_get_latest_version(void);

/**
 * @brief 检查是否有更新可用
 * @return bool true表示有更新
 */
bool ota_is_update_available(void);

/**
 * @brief 设置进度回调函数
 * @param callback 回调函数指针
 */
void ota_set_progress_callback(ota_progress_callback_t callback);

/**
 * @brief 设置状态变化回调函数
 * @param callback 回调函数指针
 */
void ota_set_state_callback(ota_state_callback_t callback);

/**
 * @brief 启用或禁用自动检查更新
 * @param enable true启用, false禁用
 */
void ota_set_auto_check(bool enable);

/**
 * @brief 强制检查更新 (忽略时间间隔)
 * @return sl_status_t 状态码
 */
sl_status_t ota_force_check_update(void);

/**
 * @brief 获取下载进度
 * @param progress 当前进度 (字节)
 * @param total 总大小 (字节)
 * @return sl_status_t 状态码
 */
sl_status_t ota_get_download_progress(uint32_t *progress, uint32_t *total);

/**
 * @brief 比较两个版本字符串
 * @param version1 版本1
 * @param version2 版本2
 * @return version_compare_result_t 比较结果
 */
version_compare_result_t ota_compare_versions(const char *version1, const char *version2);

/**
 * @brief 停止OTA任务
 * @return sl_status_t 状态码
 */
sl_status_t ota_manager_stop_task(void);

/**
 * @brief OTA任务主循环 (内部使用)
 * @param arg 任务参数
 */
void ota_task_main(void *arg);

/*==============================================*/
/**
 * 内部辅助函数声明 (用于实现)
 */

/**
 * @brief 从服务器获取版本信息
 * @param version_buffer 版本缓冲区
 * @param buffer_size 缓冲区大小
 * @return sl_status_t 状态码
 */
sl_status_t ota_fetch_version_info(char *version_buffer, size_t buffer_size);

/**
 * @brief 下载固件文件
 * @return sl_status_t 状态码
 */
sl_status_t ota_download_firmware(void);


/**
 * @brief 清除并加载证书到flash
 * @return sl_status_t 状态码
 */
sl_status_t ota_load_certificates(void);

#endif // WIFI_OTA_MANAGER_H