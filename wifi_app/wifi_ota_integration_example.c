/*******************************************************************************
* @file  wifi_ota_integration_example.c
* @brief WiFi OTA集成示例 - 展示如何在主应用程序中集成OTA功能
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

#include "wifi_ota_manager.h"
#include "wifi_ota_config.h"
#include "wifi_app.h"
#include "app_log.h"
#include "cmsis_os2.h"

/*==============================================*/
/**
 * OTA回调函数实现
 */

// OTA进度回调函数
static void ota_progress_callback_impl(uint32_t progress, uint32_t total)
{
  static uint32_t last_reported_progress = 0;

  // 只在进度变化超过5%时报告，减少日志输出
  if (progress - last_reported_progress >= 5 || progress == total) {
    app_log_info("OTA Progress: %lu/%lu (%lu%%)\r\n",
                 progress, total, (progress * 100) / total);
    last_reported_progress = progress;
  }
}

// OTA状态变化回调函数
static void ota_state_callback_impl(ota_state_t state, ota_error_t error)
{
  const char* state_strings[] = {
    "IDLE",
    "CHECKING_VERSION",
    "DOWNLOADING",
    "INSTALLING",
    "COMPLETE",
    "ERROR"
  };

  const char* error_strings[] = {
    "NONE",
    "NETWORK",
    "DNS_RESOLVE",
    "HTTP_REQUEST",
    "VERSION_PARSE",
    "DOWNLOAD_FAILED",
    "INSTALL_FAILED",
    "TIMEOUT"
  };

  if (state < sizeof(state_strings) / sizeof(state_strings[0])) {
    app_log_info("OTA State: %s", state_strings[state]);
  }

  if (error != OTA_ERROR_NONE && error < sizeof(error_strings) / sizeof(error_strings[0])) {
    app_log_error(" (Error: %s)", error_strings[error]);
  }

  app_log_info("\r\n");

  // 根据状态执行相应操作
  switch (state) {
    case OTA_STATE_COMPLETE:
      app_log_info("OTA update completed successfully! Device will restart...\r\n");
      // 可以在这里添加设备重启逻辑
      break;

    case OTA_STATE_ERROR:
      app_log_error("OTA update failed with error: %s\r\n",
                    error < sizeof(error_strings) / sizeof(error_strings[0]) ?
                    error_strings[error] : "UNKNOWN");
      break;

    default:
      break;
  }
}

/*==============================================*/
/**
 * OTA初始化函数
 */

sl_status_t wifi_ota_init(void)
{
  sl_status_t status;

  app_log_info("Initializing WiFi OTA system...\r\n");

  // 初始化OTA管理器
  status = ota_manager_init();
  if (status != SL_STATUS_OK) {
    app_log_error("Failed to initialize OTA manager: 0x%lx\r\n", status);
    return status;
  }

  // 设置回调函数
  ota_set_progress_callback(ota_progress_callback_impl);
  ota_set_state_callback(ota_state_callback_impl);

  // 加载证书
  status = ota_load_certificates();
  if (status != SL_STATUS_OK) {
    app_log_error("Failed to load OTA certificates: 0x%lx\r\n", status);
    return status;
  }

  // 启动OTA任务
  status = ota_manager_start_task();
  if (status != SL_STATUS_OK) {
    app_log_error("Failed to start OTA task: 0x%lx\r\n", status);
    return status;
  }

  // 启用自动检查
  ota_set_auto_check(true);

  app_log_info("WiFi OTA system initialized successfully\r\n");
  app_log_info("Current firmware version: %s\r\n", ota_get_current_version());

  return SL_STATUS_OK;
}

/*==============================================*/
/**
 * OTA命令处理函数 (可用于shell命令或其他触发方式)
 */

void wifi_ota_check_command(void)
{
  app_log_info("Checking for firmware updates...\r\n");

  sl_status_t status = ota_force_check_update();

  if (status != SL_STATUS_OK) {
    app_log_error("Failed to check for updates: 0x%lx\r\n", status);
    return;
  }

  // 等待检查完成
  osDelay(5000);

  if (ota_is_update_available()) {
    app_log_info("New firmware version available: %s\r\n", ota_get_latest_version());
    app_log_info("Current version: %s\r\n", ota_get_current_version());
    app_log_info("Use 'ota_update' command to start update\r\n");
  } else {
    app_log_info("Firmware is up to date\r\n");
  }
}

void wifi_ota_update_command(void)
{
  if (!ota_is_update_available()) {
    app_log_info("No firmware update available. Check for updates first.\r\n");
    return;
  }

  app_log_info("Starting firmware update...\r\n");
  app_log_info("Updating from %s to %s\r\n",
               ota_get_current_version(), ota_get_latest_version());

  sl_status_t status = ota_start_update();

  if (status != SL_STATUS_OK) {
    app_log_error("Failed to start firmware update: 0x%lx\r\n", status);
  }
}

void wifi_ota_status_command(void)
{
  ota_state_t state = ota_get_current_state();
  ota_error_t error = ota_get_last_error();

  const char* state_strings[] = {
    "IDLE - Ready for operations",
    "CHECKING_VERSION - Checking for updates",
    "DOWNLOADING - Downloading firmware",
    "INSTALLING - Installing firmware",
    "COMPLETE - Update completed",
    "ERROR - Error occurred"
  };

  app_log_info("=== OTA Status ===\r\n");
  app_log_info("Current State: %s\r\n",
               state < sizeof(state_strings) / sizeof(state_strings[0]) ?
               state_strings[state] : "UNKNOWN");

  app_log_info("Current Version: %s\r\n", ota_get_current_version());
  app_log_info("Latest Version: %s\r\n", ota_get_latest_version());
  app_log_info("Update Available: %s\r\n", ota_is_update_available() ? "Yes" : "No");

  if (error != OTA_ERROR_NONE) {
    const char* error_strings[] = {
      "NONE", "NETWORK", "DNS_RESOLVE", "HTTP_REQUEST",
      "VERSION_PARSE", "DOWNLOAD_FAILED", "INSTALL_FAILED", "TIMEOUT"
    };

    app_log_info("Last Error: %s\r\n",
                 error < sizeof(error_strings) / sizeof(error_strings[0]) ?
                 error_strings[error] : "UNKNOWN");
  }

  if (state == OTA_STATE_DOWNLOADING) {
    uint32_t progress, total;
    if (ota_get_download_progress(&progress, &total) == SL_STATUS_OK) {
      app_log_info("Download Progress: %lu/%lu (%lu%%)\r\n",
                   progress, total, total > 0 ? (progress * 100) / total : 0);
    }
  }

  app_log_info("==================\r\n");
}

/*==============================================*/
/**
 * 在主应用程序中集成OTA的示例
 */

void wifi_ota_integration_example(void)
{
  /*
   * 在主应用程序的初始化函数中调用以下代码：
   *
   * // 等待WiFi连接完成
   * while (catcollar_wifi_connection_get_state() != CATCOLLAR_WIFI_CONNECTED) {
   *   osDelay(1000);
   * }
   *
   * // 初始化OTA系统
   * if (wifi_ota_init() != SL_STATUS_OK) {
   *   app_log_error("Failed to initialize OTA system\r\n");
   * }
   */

  /*
   * 在shell命令处理中添加以下命令：
   *
   * if (strcmp(command, "ota_check") == 0) {
   *   wifi_ota_check_command();
   * } else if (strcmp(command, "ota_update") == 0) {
   *   wifi_ota_update_command();
   * } else if (strcmp(command, "ota_status") == 0) {
   *   wifi_ota_status_command();
   * }
   */

  /*
   * 如果需要在程序启动时自动检查更新：
   *
   * // 在WiFi连接后延迟一段时间再检查更新
   * osDelay(10000); // 等待10秒
   * wifi_ota_check_command();
   */
}

/*==============================================*/
/**
 * 配置修改说明
 */

/*
 * 1. 修改 wifi_ota_config.h 中的配置：
 *    - AWS_S3_BUCKET_HOST: 你的S3存储桶域名
 *    - FIRMWARE_VERSION_FILE: 版本文件名
 *    - FIRMWARE_BINARY_FILE: 固件文件名
 *    - CURRENT_FIRMWARE_VERSION: 当前固件版本
 *
 * 2. 在项目的构建配置中添加版本定义：
 *    在编译器选项中添加: -DCURRENT_FIRMWARE_VERSION="1.0.0"
 *
 * 3. 确保项目中包含以下SDK功能：
 *    - HTTP客户端
 *    - SSL/TLS支持
 *    - DNS客户端
 *    - HTTP OTAF支持
 *
 * 4. AWS S3存储桶结构建议：
 *    your-bucket/
 *    ├── version.txt          (包含最新版本号，如: 1.1.0)
 *    └── catcollar-mainboard.bin  (固件二进制文件)
 *
 * 5. S3存储桶权限配置：
 *    - 设置为公共读取权限，或配置适当的访问策略
 *    - 确保能够通过HTTPS访问文件
 */