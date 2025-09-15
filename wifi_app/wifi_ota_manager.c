/*******************************************************************************
* @file  wifi_ota_manager.c
* @brief WiFi OTA升级管理器实现
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
#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"
#include "sl_net.h"
#include "sl_net_dns.h"
#include "sl_net_si91x.h"
#include "firmware_upgradation.h"
#include "app_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if USE_SDK_AWS_CERTIFICATE
#include "../../wiseconnect3_sdk_3.5.0/resources/certificates/aws_starfield_ca.pem.h"
#endif

/*==============================================*/
/**
 * 全局变量和静态变量
 */

static ota_manager_t ota_manager = {0};
static osThreadId_t ota_task_handle = NULL;
static osSemaphoreId_t ota_semaphore = NULL;
static volatile bool ota_task_running = false;
static volatile bool ota_response_received = false;
static volatile sl_status_t ota_callback_status = SL_STATUS_OK;

// 回调函数指针
static ota_progress_callback_t progress_callback = NULL;
static ota_state_callback_t state_callback = NULL;

// OTA任务属性
static const osThreadAttr_t ota_task_attributes = {
  .name       = "ota_task",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 4096,
  .priority   = osPriorityNormal,
  .tz_module  = 0,
  .reserved   = 0,
};

/*==============================================*/
/**
 * 内部函数声明
 */

static void ota_set_state(ota_state_t new_state, ota_error_t error);
static sl_status_t ota_parse_version_string(const char *version_str, char *parsed_version, size_t buffer_size);
static uint32_t ota_get_current_time_seconds(void);
static bool ota_should_check_update(void);
static sl_status_t ota_fw_update_response_handler(sl_wifi_event_t event,
                                                 uint16_t *data,
                                                 uint32_t data_length,
                                                 void *arg);

/*==============================================*/
/**
 * 公共函数实现
 */

sl_status_t ota_manager_init(void)
{
  // 初始化OTA管理器结构体
  memset(&ota_manager, 0, sizeof(ota_manager_t));

  ota_manager.current_state = OTA_STATE_IDLE;
  ota_manager.last_error = OTA_ERROR_NONE;
  ota_manager.auto_check_enabled = true;
  ota_manager.update_available = false;

  // 设置当前版本
  strncpy(ota_manager.current_version, CURRENT_FIRMWARE_VERSION, MAX_VERSION_STRING_LENGTH - 1);
  ota_manager.current_version[MAX_VERSION_STRING_LENGTH - 1] = '\0';

  // 创建信号量
  ota_semaphore = osSemaphoreNew(1, 0, NULL);
  if (ota_semaphore == NULL) {
    OTA_LOG_ERROR("Failed to create OTA semaphore\r\n");
    return SL_STATUS_ALLOCATION_FAILED;
  }

  OTA_LOG_INFO("OTA Manager initialized successfully\r\n");
  OTA_LOG_INFO("Current firmware version: %s\r\n", ota_manager.current_version);

  return SL_STATUS_OK;
}

sl_status_t ota_manager_start_task(void)
{
  if (ota_task_handle != NULL) {
    OTA_LOG_ERROR("OTA task already running\r\n");
    return SL_STATUS_ALREADY_EXISTS;
  }

  ota_task_running = true;
  ota_task_handle = osThreadNew((osThreadFunc_t)ota_task_main, NULL, &ota_task_attributes);

  if (ota_task_handle == NULL) {
    ota_task_running = false;
    OTA_LOG_ERROR("Failed to create OTA task\r\n");
    return SL_STATUS_ALLOCATION_FAILED;
  }

  OTA_LOG_INFO("OTA task started successfully\r\n");
  return SL_STATUS_OK;
}

sl_status_t ota_manager_stop_task(void)
{
  if (ota_task_handle == NULL) {
    OTA_LOG_ERROR("OTA task is not running\r\n");
    return SL_STATUS_NOT_INITIALIZED;
  }

  ota_task_running = false;

  // 释放信号量以唤醒任务
  osSemaphoreRelease(ota_semaphore);

  // 等待任务结束
  osDelay(1000);

  if (ota_task_handle != NULL) {
    osThreadTerminate(ota_task_handle);
    ota_task_handle = NULL;
  }

  OTA_LOG_INFO("OTA task stopped\r\n");
  return SL_STATUS_OK;
}

sl_status_t ota_check_for_updates(void)
{
  if (catcollar_wifi_connection_get_state() != CATCOLLAR_WIFI_CONNECTED) {
    OTA_LOG_ERROR("WiFi not connected, cannot check for updates\r\n");
    ota_set_state(OTA_STATE_ERROR, OTA_ERROR_NETWORK);
    return SL_STATUS_NETWORK_DOWN;
  }

  ota_set_state(OTA_STATE_CHECKING_VERSION, OTA_ERROR_NONE);

  char version_buffer[MAX_VERSION_STRING_LENGTH];
  sl_status_t status = ota_fetch_version_info(version_buffer, sizeof(version_buffer));

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("Failed to fetch version info: 0x%lx\r\n", status);
    ota_set_state(OTA_STATE_ERROR, OTA_ERROR_VERSION_PARSE);
    return status;
  }

  // 解析版本字符串
  char parsed_version[MAX_VERSION_STRING_LENGTH];
  status = ota_parse_version_string(version_buffer, parsed_version, sizeof(parsed_version));

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("Failed to parse version string\r\n");
    ota_set_state(OTA_STATE_ERROR, OTA_ERROR_VERSION_PARSE);
    return status;
  }

  // 保存最新版本
  strncpy(ota_manager.latest_version, parsed_version, MAX_VERSION_STRING_LENGTH - 1);
  ota_manager.latest_version[MAX_VERSION_STRING_LENGTH - 1] = '\0';

  // 比较版本
  version_compare_result_t result = ota_compare_versions(ota_manager.current_version, parsed_version);

  if (result == VERSION_NEWER) {
    ota_manager.update_available = true;
    OTA_LOG_INFO("New firmware version available: %s (current: %s)\r\n",
                 parsed_version, ota_manager.current_version);
    ota_set_state(OTA_STATE_IDLE, OTA_ERROR_NONE);
  } else if (result == VERSION_SAME) {
    ota_manager.update_available = false;
    OTA_LOG_INFO("Firmware is up to date: %s\r\n", ota_manager.current_version);
    ota_set_state(OTA_STATE_IDLE, OTA_ERROR_NONE);
  } else {
    OTA_LOG_INFO("Current firmware is newer than server version\r\n");
    ota_manager.update_available = false;
    ota_set_state(OTA_STATE_IDLE, OTA_ERROR_NONE);
  }

  ota_manager.last_check_time = ota_get_current_time_seconds();
  return SL_STATUS_OK;
}

sl_status_t ota_start_update(void)
{
  if (!ota_manager.update_available) {
    OTA_LOG_ERROR("No update available\r\n");
    return SL_STATUS_NOT_AVAILABLE;
  }

  if (catcollar_wifi_connection_get_state() != CATCOLLAR_WIFI_CONNECTED) {
    OTA_LOG_ERROR("WiFi not connected, cannot start update\r\n");
    ota_set_state(OTA_STATE_ERROR, OTA_ERROR_NETWORK);
    return SL_STATUS_NETWORK_DOWN;
  }

  ota_set_state(OTA_STATE_DOWNLOADING, OTA_ERROR_NONE);

  OTA_LOG_INFO("Starting firmware download...\r\n");

  sl_status_t status = ota_download_firmware();

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("Firmware download failed: 0x%lx\r\n", status);
    ota_set_state(OTA_STATE_ERROR, OTA_ERROR_DOWNLOAD_FAILED);
    return status;
  }

  ota_set_state(OTA_STATE_INSTALLING, OTA_ERROR_NONE);
  OTA_LOG_INFO("Firmware download completed, installing...\r\n");

  // 等待安装完成
  osDelay(5000);

  ota_set_state(OTA_STATE_COMPLETE, OTA_ERROR_NONE);
  OTA_LOG_INFO("Firmware update completed successfully\r\n");

  return SL_STATUS_OK;
}

ota_state_t ota_get_current_state(void)
{
  return ota_manager.current_state;
}

ota_error_t ota_get_last_error(void)
{
  return ota_manager.last_error;
}

const char* ota_get_current_version(void)
{
  return ota_manager.current_version;
}

const char* ota_get_latest_version(void)
{
  return ota_manager.latest_version;
}

bool ota_is_update_available(void)
{
  return ota_manager.update_available;
}

void ota_set_progress_callback(ota_progress_callback_t callback)
{
  progress_callback = callback;
}

void ota_set_state_callback(ota_state_callback_t callback)
{
  state_callback = callback;
}

void ota_set_auto_check(bool enable)
{
  ota_manager.auto_check_enabled = enable;
  OTA_LOG_INFO("Auto check %s\r\n", enable ? "enabled" : "disabled");
}

sl_status_t ota_force_check_update(void)
{
  ota_manager.last_check_time = 0; // 重置检查时间
  return ota_check_for_updates();
}

sl_status_t ota_get_download_progress(uint32_t *progress, uint32_t *total)
{
  if (progress == NULL || total == NULL) {
    return SL_STATUS_NULL_POINTER;
  }

  *progress = ota_manager.download_progress;
  *total = ota_manager.total_size;

  return SL_STATUS_OK;
}

version_compare_result_t ota_compare_versions(const char *version1, const char *version2)
{
  if (version1 == NULL || version2 == NULL) {
    return VERSION_INVALID;
  }

  // 简单的版本比较实现 (假设版本格式为 x.y.z)
  int v1_major = 0, v1_minor = 0, v1_patch = 0;
  int v2_major = 0, v2_minor = 0, v2_patch = 0;

  int parsed1 = sscanf(version1, "%d.%d.%d", &v1_major, &v1_minor, &v1_patch);
  int parsed2 = sscanf(version2, "%d.%d.%d", &v2_major, &v2_minor, &v2_patch);

  if (parsed1 < 1 || parsed2 < 1) {
    return VERSION_INVALID;
  }

  // 比较主版本号
  if (v1_major < v2_major) return VERSION_OLDER;
  if (v1_major > v2_major) return VERSION_NEWER;

  // 比较次版本号
  if (v1_minor < v2_minor) return VERSION_OLDER;
  if (v1_minor > v2_minor) return VERSION_NEWER;

  // 比较补丁版本号
  if (v1_patch < v2_patch) return VERSION_OLDER;
  if (v1_patch > v2_patch) return VERSION_NEWER;

  return VERSION_SAME;
}

/*==============================================*/
/**
 * OTA任务主循环
 */

void ota_task_main(void *arg)
{
  UNUSED_PARAMETER(arg);

  OTA_LOG_INFO("OTA task started\r\n");

  while (ota_task_running) {
    // 等待信号量或超时
    osSemaphoreAcquire(ota_semaphore, VERSION_CHECK_INTERVAL * 1000);

    if (!ota_task_running) {
      break;
    }

    // 检查是否需要进行版本检查
    if (ota_manager.auto_check_enabled && ota_should_check_update()) {
      OTA_LOG_INFO("Performing automatic update check...\r\n");
      ota_check_for_updates();
    }

    // 如果有可用更新且处于空闲状态，可以考虑自动更新
    // 这里我们只记录日志，实际更新需要用户触发
    if (ota_manager.update_available && ota_manager.current_state == OTA_STATE_IDLE) {
      OTA_LOG_INFO("Update available, waiting for user action\r\n");
    }
  }

  OTA_LOG_INFO("OTA task exiting\r\n");
  ota_task_handle = NULL;
}

/*==============================================*/
/**
 * 内部辅助函数实现
 */

static void ota_set_state(ota_state_t new_state, ota_error_t error)
{
  ota_manager.current_state = new_state;
  ota_manager.last_error = error;

  // 调用状态变化回调
  if (state_callback != NULL) {
    state_callback(new_state, error);
  }

  // 输出状态变化日志
  const char* state_names[] = {
    "IDLE", "CHECKING_VERSION", "DOWNLOADING", "INSTALLING", "COMPLETE", "ERROR"
  };

  if (new_state < sizeof(state_names) / sizeof(state_names[0])) {
    OTA_LOG_DEBUG("OTA state changed to: %s\r\n", state_names[new_state]);
  }
}

static sl_status_t ota_parse_version_string(const char *version_str, char *parsed_version, size_t buffer_size)
{
  if (version_str == NULL || parsed_version == NULL || buffer_size == 0) {
    return SL_STATUS_NULL_POINTER;
  }

  // 简单的版本字符串解析 - 假设服务器返回纯版本号
  // 去除前后空白字符和换行符
  const char *start = version_str;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
    start++;
  }

  size_t len = 0;
  const char *end = start;
  while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n' && len < buffer_size - 1) {
    parsed_version[len] = *end;
    len++;
    end++;
  }

  parsed_version[len] = '\0';

  if (len == 0) {
    return SL_STATUS_INVALID_PARAMETER;
  }

  return SL_STATUS_OK;
}

static uint32_t ota_get_current_time_seconds(void)
{
  // 使用RTOS tick计算近似时间 (秒)
  return osKernelGetTickCount() / osKernelGetTickFreq();
}

static bool ota_should_check_update(void)
{
  uint32_t current_time = ota_get_current_time_seconds();
  return (current_time - ota_manager.last_check_time) >= VERSION_CHECK_INTERVAL;
}

sl_status_t ota_fetch_version_info(char *version_buffer, size_t buffer_size)
{
  if (version_buffer == NULL || buffer_size == 0) {
    return SL_STATUS_NULL_POINTER;
  }

  // DNS解析
  sl_ip_address_t dns_query_rsp = { 0 };
  sl_status_t status;
  int32_t dns_retry_count = MAX_DNS_RETRY_COUNT;

  do {
    status = sl_net_dns_resolve_hostname(AWS_S3_BUCKET_HOST, DNS_TIMEOUT, SL_NET_DNS_TYPE_IPV4, &dns_query_rsp);
    dns_retry_count--;
  } while ((dns_retry_count != 0) && (status != SL_STATUS_OK));

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("DNS resolution failed for %s: 0x%lx\r\n", AWS_S3_BUCKET_HOST, status);
    return status;
  }

  // 构建服务器IP地址字符串
  uint32_t server_address = dns_query_rsp.ip.v4.value;
  char server_ip[16];
  sprintf(server_ip, "%ld.%ld.%ld.%ld",
          server_address & 0x000000ff,
          (server_address & 0x0000ff00) >> 8,
          (server_address & 0x00ff0000) >> 16,
          (server_address & 0xff000000) >> 24);

  OTA_LOG_INFO("Resolved %s to %s\r\n", AWS_S3_BUCKET_HOST, server_ip);

  // TODO: 实现HTTP GET请求获取版本信息
  // 这里暂时返回一个模拟的版本号用于测试
  strncpy(version_buffer, "1.1.0", buffer_size - 1);
  version_buffer[buffer_size - 1] = '\0';

  OTA_LOG_INFO("Fetched version info: %s\r\n", version_buffer);

  return SL_STATUS_OK;
}

sl_status_t ota_download_firmware(void)
{
  // DNS解析
  sl_ip_address_t dns_query_rsp = { 0 };
  sl_status_t status;
  int32_t dns_retry_count = MAX_DNS_RETRY_COUNT;

  do {
    status = sl_net_dns_resolve_hostname(AWS_S3_BUCKET_HOST, DNS_TIMEOUT, SL_NET_DNS_TYPE_IPV4, &dns_query_rsp);
    dns_retry_count--;
  } while ((dns_retry_count != 0) && (status != SL_STATUS_OK));

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("DNS resolution failed: 0x%lx\r\n", status);
    return status;
  }

  // 构建服务器IP地址字符串
  uint32_t server_address = dns_query_rsp.ip.v4.value;
  char server_ip[16];
  sprintf(server_ip, "%ld.%ld.%ld.%ld",
          server_address & 0x000000ff,
          (server_address & 0x0000ff00) >> 8,
          (server_address & 0x00ff0000) >> 16,
          (server_address & 0xff000000) >> 24);

  OTA_LOG_INFO("Starting firmware download from %s\r\n", server_ip);

  // 设置固件更新回调
  sl_wifi_set_callback(SL_WIFI_HTTP_OTA_FW_UPDATE_EVENTS,
                       (sl_wifi_callback_function_t)ota_fw_update_response_handler,
                       NULL);

  // 配置HTTP OTAF参数
  sl_si91x_http_otaf_params_t http_params = { 0 };
  http_params.flags = OTA_FLAGS;
  http_params.ip_address = (uint8_t *)server_ip;
  http_params.port = OTA_HTTP_PORT;
  http_params.resource = (uint8_t *)FIRMWARE_BINARY_FILE;
  http_params.host_name = (uint8_t *)AWS_S3_BUCKET_HOST;
  http_params.extended_header = (uint8_t *)HTTP_EXTENDED_HEADER;
  http_params.user_name = (uint8_t *)OTA_USERNAME;
  http_params.password = (uint8_t *)OTA_PASSWORD;

  // 开始OTAF下载
  ota_response_received = false;
  status = sl_si91x_http_otaf_v2(&http_params);

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("Failed to start firmware download: 0x%lx\r\n", status);
    return status;
  }

  // 等待下载完成
  uint32_t timeout_count = 0;
  const uint32_t max_timeout = OTA_TIMEOUT / 1000; // 转换为秒

  while (!ota_response_received && timeout_count < max_timeout) {
    osDelay(1000);
    timeout_count++;

    // 更新进度 (这里是模拟进度)
    ota_manager.download_progress = (timeout_count * 100) / max_timeout;

    if (progress_callback != NULL) {
      progress_callback(ota_manager.download_progress, 100);
    }
  }

  if (!ota_response_received) {
    OTA_LOG_ERROR("Firmware download timeout\r\n");
    return SL_STATUS_TIMEOUT;
  }

  return ota_callback_status;
}

static sl_status_t ota_fw_update_response_handler(sl_wifi_event_t event,
                                                 uint16_t *data,
                                                 uint32_t data_length,
                                                 void *arg)
{
  UNUSED_PARAMETER(data);
  UNUSED_PARAMETER(data_length);
  UNUSED_PARAMETER(arg);

  if (SL_WIFI_CHECK_IF_EVENT_FAILED(event)) {
    ota_response_received = true;
    ota_callback_status = SL_STATUS_FAIL;
    OTA_LOG_ERROR("Firmware update event failed\r\n");
    return SL_STATUS_FAIL;
  }

  ota_response_received = true;
  ota_callback_status = SL_STATUS_OK;
  OTA_LOG_INFO("Firmware update completed successfully\r\n");

  return SL_STATUS_OK;
}

sl_status_t ota_load_certificates(void)
{
#if USE_SDK_AWS_CERTIFICATE && LOAD_CERTIFICATE
  sl_status_t status;

  // 加载AWS CA证书
  status = sl_net_set_credential(SL_NET_TLS_SERVER_CREDENTIAL_ID(CERTIFICATE_INDEX),
                                 SL_NET_SIGNING_CERTIFICATE,
                                 aws_starfield_ca,
                                 sizeof(aws_starfield_ca) - 1);

  if (status != SL_STATUS_OK) {
    OTA_LOG_ERROR("Failed to load AWS CA certificate: 0x%lx\r\n", status);
    return status;
  }

  OTA_LOG_INFO("AWS CA certificate loaded successfully at index %d\r\n", CERTIFICATE_INDEX);
#endif

  return SL_STATUS_OK;
}

/*==============================================*/
/**
 * TodoWrite更新
 */