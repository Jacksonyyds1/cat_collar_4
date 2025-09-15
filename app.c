/*******************************************************************************
* @file  app.c
* @brief
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
/*************************************************************************
 *
 */

/*================================================================================
 * @brief : This file contains example application for Wlan Station BLE
 * Provisioning
 * @section Description :
 * This application explains how to get the WLAN connection functionality using
 * BLE provisioning.
 * Silicon Labs Module starts advertising and with BLE Provisioning the Access Point
 * details are fetched.
 * Silicon Labs device is configured as a WiFi station and connects to an Access Point.
 =================================================================================*/

/**
 * Include files
 **/
//! SL Wi-Fi SDK includes
#include "sl_board_configuration.h"
#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"
#include "cmsis_os2.h"
#include "sl_utility.h"
#include "app.h"

//BLE Specific inclusions
#include <rsi_ble_apis.h>
#include "ble_config.h"
#include "rsi_ble_common_config.h"

#include <rsi_common_apis.h>
#include "app_log.h"
#include "common_i2c.h"
#include "common.h"
#include "blinky.h"
#include "lsm6dsv.h"
#include "spa06_003.h"
#include "bmm350.h" 
#include "sy6105_bsp.h"
#include "mlx90632.h"
#include "spi_flash.h"
#include "storage_api.h"
#include "syscfg_id.h"
#include "shell_port.h"

#include "chekr_record.h"

#include "ble_data_parse.h"
#include "ble_app_cat.h"
#include "wifi_http_event.h"
#include "wifi_app.h"

// #define CATCOLLAR_DEBUG

static int32_t prev_heap = -1;

#ifdef CATCOLLAR_DEBUG
#define LOG_HEAP_DELTA(step) do { \
    int32_t current_heap = xPortGetFreeHeapSize(); \
    int32_t heap_delta = (prev_heap == -1) ? 0 : (prev_heap - current_heap); \
    app_log_info(#step " Free heap size: %d bytes, Delta: %d bytes\r\n", current_heap, heap_delta); \
    prev_heap = current_heap; \
} while (0)
#else
#define LOG_HEAP_DELTA(step) do {} while (0)
#endif

// Function prototypes
extern void wifi_app_task(void);
extern void rsi_ble_configurator_task(void *argument);
void rsi_ble_configurator_init(void);
uint8_t magic_word;

osSemaphoreId_t wlan_thread_sem;
osSemaphoreId_t ble_thread_sem;

const osThreadAttr_t thread_attributes = {
  .name       = "application_thread",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  // .stack_size = 3072,
  .stack_size = 6144,
  .priority   = osPriorityNormal,
  .tz_module  = 0,
  .reserved   = 0,
};

const osThreadAttr_t ble_thread_attributes = {
  .name       = "ble_thread",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 2048,
  .priority   = osPriorityNormal1,
  .tz_module  = 0,
  .reserved   = 0,
};

const osThreadAttr_t wifi_thread_attributes = {
  .name       = "wifi_thread",
  .attr_bits  = 0,
  .cb_mem     = 0,
  .cb_size    = 0,
  .stack_mem  = 0,
  .stack_size = 2048,
  .priority   = osPriorityNormal,
  .tz_module  = 0,
  .reserved   = 0,
};

#define TASK_SENSOR_DATA_STACK_SIZE   1024
static StackType_t sensordataTaskStack[TASK_SENSOR_DATA_STACK_SIZE];
static StaticTask_t sensordataTaskHandle;
void sensordatareadTask(void *argument);

void sensor_data_thread_create(void)
{
    app_log_debug("Sensor data read task is running...\r\n");
 xTaskCreateStatic(
   sensordatareadTask,           // 任务函数
   "sensor_data_read_Task",         // 任务名称
   TASK_SENSOR_DATA_STACK_SIZE,    // 堆栈大小
   NULL,                   // 任务参数
   osPriorityNormal2,                     // 任务优先级 configMAX_PRIORITIES
   sensordataTaskStack,      // 静态任务堆栈
   &sensordataTaskHandle     // 静态任务控制块
 );
}

void sensordatareadTask(void *argument)
{
  // UNUSED_PARAMETER(argument);
  // while (1)
  // {
  //   osDelay(1000);
  //   // lsm6dsv_test();
  //   // 打印时间戳测试
  //   // app_log_info("current time: %s\r\n", get_unix_timestamp_string());
  //   // bmm350_test();
  //   // spa06_test();
  //   // mlx90632_test();
  // }

  UNUSED_PARAMETER(argument);
  static uint8_t test_phase = 0;
  static uint32_t start_time = 0;
  
  while (1)
  {
    osDelay(100);
    
    switch(test_phase) {
      // 阶段 0: 开始记录数据
      case 0:
        app_log_info("Starting data recording...\r\n");
        record_to_file("active1", RECORD_START);
        start_time = osKernelGetTickCount();
        test_phase = 1;
        break;

      // 阶段 1: 等待 10 秒后停止记录
      case 1:
        if ((osKernelGetTickCount() - start_time) > 10000) { // 10 秒后
          app_log_info("Stopping data recording...\r\n");
          record_to_file("active", RECORD_STOP);
          test_phase = 2;
        }
        break;

      // 阶段 2: 读取并验证记录的数据
      case 2:
        app_log_info("Reading recorded data for testing...\r\n");
        test_read_recorded_data();
        test_phase = 4; // 完成测试
        break;

      case 3:
        app_log_info("Cleaning up test files...\r\n");
        cleanup_test_files();
        app_log_info("===== DATA RECORDING TEST COMPLETED =====\r\n");
        test_phase = 4; // 完成测试
        break;
        
      default:
        // 保持空闲状态
        break;
    }

    if (test_phase == 4) {
      app_log_info("===== Deleting sensordatareadTask =====");
      vTaskDelete(NULL); // 删除任务
    } else {
      vTaskDelay(100 / portTICK_PERIOD_MS); // 等待 100ms
    }

  }
}

#include "sl_net.h"
#include "sl_net_constants.h"

void application(void *argument)
{
   UNUSED_PARAMETER(argument);
  
  // app_log_init();

   prev_heap = xPortGetFreeHeapSize();
   app_log_info("Init Free heap size: %d bytes\r\n", prev_heap);

  int32_t status                     = RSI_SUCCESS;
  sl_wifi_firmware_version_t version = { 0 };
//  sl_mac_address_t mac_addr          = { 0 };
  //! Wi-Fi initialization
  status = sl_wifi_init(&cat_wifi_client_config, NULL, sl_wifi_default_event_handler);
  // status = sl_wifi_init(&station_init_configuration, NULL, sl_wifi_default_event_handler);
  // status = sl_net_init(SL_NET_WIFI_CLIENT_INTERFACE, &cat_wifi_config, NULL, sl_wifi_default_event_handler);
  
  if (status != SL_STATUS_OK) {
    LOG_PRINT("\r\nWi-Fi Initialization Failed, Error Code : 0x%lX\r\n", status);
    return;
  }
  printf("\r\n Wi-Fi initialization is successful\r\n");

  // status = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, &mac_addr);
  // if (status == SL_STATUS_OK) {
  //   printf("Device MAC address: %x:%x:%x:%x:%x:%x\r\n",
  //          mac_addr.octet[0],
  //          mac_addr.octet[1],
  //          mac_addr.octet[2],
  //          mac_addr.octet[3],
  //          mac_addr.octet[4],
  //          mac_addr.octet[5]);
  // } else {
  //   app_log_error("Failed to get mac address: 0x%lx\r\n", status);
  // }

  //! Firmware version Prints
  status = sl_wifi_get_firmware_version(&version);
  if (status != SL_STATUS_OK) {
    printf("\r\nFirmware version Failed, Error Code : 0x%lX\r\n", status);
  } else {
    print_firmware_version(&version);
  }

  // status = sl_net_up(SL_NET_WIFI_CLIENT_INTERFACE, SL_NET_DEFAULT_WIFI_CLIENT_PROFILE_ID);
  // if (status != SL_STATUS_OK) {
  //   app_log_error("Failed to bring Wi-Fi client interface up: 0x%lx\r\n", status);
  //   return;
  // }
  // printf("\r\nWi-Fi client connected\r\n");

LOG_HEAP_DELTA(1);
  wlan_thread_sem = osSemaphoreNew(1, 0, NULL);
  if (wlan_thread_sem == NULL) {
    app_log_error("Failed to create wlan_thread_sem\n");
    return;
  }

  ble_thread_sem = osSemaphoreNew(1, 0, NULL);
  if (ble_thread_sem == NULL) {
    app_log_error("Failed to create ble_thread_sem\n");
    return;
  }

  if (osThreadNew((osThreadFunc_t)rsi_ble_configurator_task, NULL, &ble_thread_attributes) == NULL) {
    app_log_error("Failed to create BLE thread\n");
  }
LOG_HEAP_DELTA(2);
  // BLE initialization
  rsi_ble_configurator_init();

  if (osThreadNew((osThreadFunc_t)wifi_app_task, NULL, &wifi_thread_attributes) == NULL) {
    app_log_error("Failed to create wifi thread\n");
  }

  app_log_info("\r\n");
  app_log_info(" \\ | /\r\n");
  app_log_info("- SL -     Thread Operating System\r\n");
  app_log_info(" / | \\     %s build %s %s\r\n",
              get_app_version(), __DATE__, __TIME__);
  app_log_info("==================================\r\n");

  platform_i2c_init(INSTANCE_ZERO);
  platform_i2c_init(INSTANCE_ONE);
  platform_i2c_init(INSTANCE_TWO);
LOG_HEAP_DELTA(3);
  leds_init();

  spi_flash_init();
  // spi_flash_format();    //格式化SPI Flash
LOG_HEAP_DELTA(4);
  // 挂载文件系统
  storage_init();
  //需要再挂载文件后初始化时间戳
  unix_timestamp_init();
LOG_HEAP_DELTA(5);
  restore_syscfg_config();

  lsm6dsv_init();
  // spa06_init();
  // sy6105_init();
  // bmm350_init();
  // mlx90632_init();
  // spi_flash_test();
  
  // shellPortInit();

  // record_to_file("active", RECORD_START);

  // test_file_close();
// LOG_HEAP_DELTA(6);
  // sensor_data_thread_create();

  ble_data_parse_task_init();

  chekr_service_init();

  leds_play(BLUE_LED, LEDS_SLOW_BLINK);
  leds_play(GREEN_LED, LEDS_SLOW_BLINK);

  // wifi_connect_test();

  // https_upload_test();

#ifdef CATCOLLAR_DEBUG   // 调试时打开，监测栈空间剩余量
  static uint32_t last_heap_print_tick = 0;
  const uint32_t heap_print_interval = 500; // 500ms
#endif

  for (;;){
#ifdef CATCOLLAR_DEBUG   // 调试时打开，监测栈空间剩余量
    // uint32_t hightWaterMark =  uxTaskGetStackHighWaterMark(NULL);
    // app_log_debug("AppInit->hightWaterMark=%d\r\n", hightWaterMark);
    // 每500ms打印一次堆内存信息
    uint32_t current_tick = osKernelGetTickCount();
    if (current_tick - last_heap_print_tick >= heap_print_interval) {
      last_heap_print_tick = current_tick;
      
      // 获取并打印堆内存信息
      size_t free_heap = xPortGetFreeHeapSize();
      size_t min_heap = xPortGetMinimumEverFreeHeapSize();
      
      app_log_info("Heap Status: Free=%d bytes, MinEver=%d bytes\r\n", 
        free_heap, min_heap);
      }
      
      // 添加短暂延迟以降低CPU占用
      osDelay(50);
#endif

    // blinky_process_action();
  }
  

  return;
}

void app_init(void)
{
  osThreadNew((osThreadFunc_t)application, NULL, &thread_attributes);
}
