/*******************************************************************************
* @file  wifi_ota_simple_example.c
* @brief 简化的WiFi OTA集成示例
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
 * 简化的OTA集成示例
 *
 * 这个文件展示了如何在现有项目中最简单地集成OTA功能
 */

// 简单的进度回调
void simple_ota_progress_callback(uint32_t progress, uint32_t total)
{
    // 每10%报告一次进度
    static uint32_t last_percent = 0;
    uint32_t current_percent = (progress * 100) / total;

    if (current_percent >= last_percent + 10 || progress == total) {
        app_log_info("OTA Progress: %lu%%\r\n", current_percent);
        last_percent = current_percent;
    }
}

// 简单的状态回调
void simple_ota_state_callback(ota_state_t state, ota_error_t error)
{
    switch (state) {
        case OTA_STATE_IDLE:
            app_log_info("OTA: Ready\r\n");
            break;
        case OTA_STATE_CHECKING_VERSION:
            app_log_info("OTA: Checking version...\r\n");
            break;
        case OTA_STATE_DOWNLOADING:
            app_log_info("OTA: Downloading firmware...\r\n");
            break;
        case OTA_STATE_INSTALLING:
            app_log_info("OTA: Installing firmware...\r\n");
            break;
        case OTA_STATE_COMPLETE:
            app_log_info("OTA: Update completed successfully!\r\n");
            break;
        case OTA_STATE_ERROR:
            app_log_error("OTA: Error occurred\r\n");
            break;
    }

    if (error != OTA_ERROR_NONE) {
        app_log_error("OTA Error code: %d\r\n", error);
    }
}

/*==============================================*/
/**
 * 初始化OTA系统的简单函数
 * 在WiFi连接成功后调用
 */
sl_status_t simple_ota_init(void)
{
    sl_status_t status;

    app_log_info("Initializing OTA system...\r\n");

    // 1. 初始化OTA管理器
    status = ota_manager_init();
    if (status != SL_STATUS_OK) {
        app_log_error("OTA manager init failed: 0x%lx\r\n", status);
        return status;
    }

    // 2. 设置回调函数
    ota_set_progress_callback(simple_ota_progress_callback);
    ota_set_state_callback(simple_ota_state_callback);

    // 3. 加载证书
    status = ota_load_certificates();
    if (status != SL_STATUS_OK) {
        app_log_error("OTA certificate load failed: 0x%lx\r\n", status);
        return status;
    }

    // 4. 启动OTA任务
    status = ota_manager_start_task();
    if (status != SL_STATUS_OK) {
        app_log_error("OTA task start failed: 0x%lx\r\n", status);
        return status;
    }

    // 5. 启用自动检查
    ota_set_auto_check(true);

    app_log_info("OTA system initialized successfully\r\n");
    app_log_info("Current version: %s\r\n", ota_get_current_version());

    return SL_STATUS_OK;
}

/*==============================================*/
/**
 * 手动检查更新的简单函数
 */
void simple_ota_check_update(void)
{
    app_log_info("Checking for firmware updates...\r\n");

    sl_status_t status = ota_check_for_updates();
    if (status == SL_STATUS_OK) {
        // 等待检查完成
        osDelay(5000);

        if (ota_is_update_available()) {
            app_log_info("Update available: %s -> %s\r\n",
                        ota_get_current_version(),
                        ota_get_latest_version());
        } else {
            app_log_info("No update available\r\n");
        }
    } else {
        app_log_error("Update check failed: 0x%lx\r\n", status);
    }
}

/*==============================================*/
/**
 * 手动开始更新的简单函数
 */
void simple_ota_start_update(void)
{
    if (!ota_is_update_available()) {
        app_log_info("No update available\r\n");
        return;
    }

    app_log_info("Starting firmware update...\r\n");

    sl_status_t status = ota_start_update();
    if (status != SL_STATUS_OK) {
        app_log_error("Update start failed: 0x%lx\r\n", status);
    }
}

/*==============================================*/
/**
 * 在wifi_app.c的WIFI_APP_IPCONFIG_DONE_STATE中调用的集成函数
 */
void integrate_ota_to_wifi_app(void)
{
    static bool ota_initialized = false;

    if (!ota_initialized) {
        // 等待网络稳定
        osDelay(3000);

        if (simple_ota_init() == SL_STATUS_OK) {
            ota_initialized = true;

            // 可选：启动后自动检查更新
            osDelay(2000);
            simple_ota_check_update();
        }
    }
}

/*==============================================*/
/**
 * 使用说明：
 *
 * 1. 在wifi_app.c中的WIFI_APP_IPCONFIG_DONE_STATE添加：
 *    integrate_ota_to_wifi_app();
 *
 * 2. 如果有shell命令系统，可以添加命令：
 *    - "ota_check" -> simple_ota_check_update()
 *    - "ota_update" -> simple_ota_start_update()
 *
 * 3. 配置你的S3存储桶信息在wifi_ota_config.h中
 *
 * 4. 确保编译选项包含版本定义：
 *    -DCURRENT_FIRMWARE_VERSION="1.0.0"
 */