#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "semphr.h"
#include "timers.h"

#include <string.h>
#include <stdlib.h>
#include "ble_app_cat.h"
#include "ble_data_parse.h"
#include "chekr_dash.h"
#include "common.h"
// #include "chekr.h"
// #include "pmic_stub.h"
// #include "utils.h"
// #include "app_version.h"

#define MAX_INTERVAL_PERIOD_S (240)

typedef struct __attribute__((__packed__)) {
    frame_format_header_t header;
    uint8_t ack;
    uint8_t device_name[8];
    uint8_t device_type;
    uint8_t app_fw_major;
    uint8_t app_fw_minor;
    uint8_t app_fw_patch;
    uint8_t battery_level;
    uint16_t battery_voltage;
    int16_t input_current;
    uint16_t temperature;
    uint16_t crc;
} read_dashboard_info_resp_t;

static TimerHandle_t dashboard_timer;
static SemaphoreHandle_t dashboard_wait;
static uint32_t dashboard_interval_s;

static void dashboard_timer_handler(TimerHandle_t xTimer);

#define TASK_DASHBOARD_STACK_SIZE   4096
static StackType_t dashboard_TaskStack[TASK_DASHBOARD_STACK_SIZE];
static StaticTask_t dashboard_TaskHandle;
/* dashboard task */
static void dashboard_task(void *pvParameters);

int dashboard_init(void)
{
    // 创建信号量
    dashboard_wait = xSemaphoreCreateBinary();
    if (dashboard_wait == NULL) {
        return -1;
    }

    // 创建定时器
    dashboard_timer = xTimerCreate(
        "dashboard_timer",                   // 定时器名称
        pdMS_TO_TICKS(1000),                 // 初始周期（1秒）
        pdFALSE,                             // 单次定时器
        (void*)0,                            // 定时器ID
        dashboard_timer_handler               // 回调函数
    );
    if (dashboard_timer == NULL) {
        vSemaphoreDelete(dashboard_wait);
        return -1;
    }

    if (xTaskCreateStatic(
        dashboard_task,           // 任务函数
        "dashboard_task",         // 任务名称
        TASK_DASHBOARD_STACK_SIZE,       // 堆栈大小
        NULL,                                 // 任务参数
        osPriorityNormal2,                    // 任务优先级 configMAX_PRIORITIES
        dashboard_TaskStack,      // 静态任务堆栈
        &dashboard_TaskHandle     // 静态任务控制块
        ) == NULL)
    {
        app_log_error("Failed to create \r\n");
        vSemaphoreDelete(dashboard_wait);
        xTimerDelete(dashboard_timer, 0);
        return -1;
    }

    return 0;
}

double celsius_to_fahrenheit(double celsius)
{
    return (celsius * 9.0 / 5.0) + 32.0;
}

int dashboard_response(void)
{
    // send dashboard info
	read_dashboard_info_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = 0x1a,
		.header.frame_type = FT_REPORT,
		.header.cmd = READ_DASHBOARD_INFO,
		.ack = ERR_NONE,

		.app_fw_major = CATCOLLAR_APPLICATION_MAJOR_VERSION,
		.app_fw_minor = CATCOLLAR_APPLICATION_MINOR_VERSION,
		.app_fw_patch = CATCOLLAR_APPLICATION_PATCH_VERSION,
	};

	pmic_get_info_t info = {0};
	// pmic_get_info(&info);
    info.battery_pct = 86;
    info.battery_v = 3.85f;
    info.charging_a = 0.01f;
    info.temperature = 28;

	resp.battery_level = info.battery_pct;

	uint16_t battery_mv = (uint16_t)(info.battery_v * 100);
	resp.battery_voltage = sys_cpu_to_be16(battery_mv);

	uint16_t current_ma = abs((int)(info.charging_a * 100));
	resp.input_current = sys_cpu_to_be16(current_ma);

	float temperature_f = celsius_to_fahrenheit(info.temperature);
	uint16_t temperature_f_scaled = (uint16_t)(temperature_f * 100);
	resp.temperature = sys_cpu_to_be16(temperature_f_scaled);

	memcpy(resp.device_name, ble_get_local_name() + 6, sizeof(resp.device_name));
	resp.device_type = NESTLE_COMMERCIAL_PET_COLLAR;
    // resp.device_type = NESTLE_A3_DOG_COLLAR;
	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));

	// LOG_HEXDUMP_DBG((uint8_t *)&resp, sizeof(resp), "dashboard data");
    // app_log_debug("Dashboard response: \r\n");
    // app_log_hexdump_debug((uint8_t *)&resp, sizeof(resp));

	// app_log_debug("wrote dashboard info, %d bytes\r\n", sizeof(resp));
	return 0;
}

static void dashboard_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        // 等待信号量（无限等待）
        if (xSemaphoreTake(dashboard_wait, portMAX_DELAY) == pdTRUE) {

            dashboard_response();

            // 重新启动定时器（如果设置了间隔）
            if (dashboard_interval_s) {
                xTimerChangePeriod(dashboard_timer, pdMS_TO_TICKS(dashboard_interval_s * 1000), 0);
                xTimerStart(dashboard_timer, 0);
            }
        }
    }
}

// 定时器回调函数
static void dashboard_timer_handler(TimerHandle_t xTimer)
{
    (void)xTimer; // 避免未使用参数警告
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 释放信号量
    xSemaphoreGiveFromISR(dashboard_wait, &xHigherPriorityTaskWoken);
    
    // 如果需要的话执行上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int dashboard_ctrl(dashboard_ctrl_t ctrl, uint8_t interval)
{
    if (interval > MAX_INTERVAL_PERIOD_S) {
        return -1;
    }

    switch (ctrl) {
    case DASH_ONCE:
        dashboard_interval_s = 0;
        xSemaphoreGive(dashboard_wait);
        break;
    case DASH_START:
        dashboard_interval_s = interval;
        xSemaphoreGive(dashboard_wait);
        break;
    case DASH_STOP:
        dashboard_interval_s = 0;
        xTimerStop(dashboard_timer, 0);
        break;
    default:
        return -1;
    }

    return 0;
}