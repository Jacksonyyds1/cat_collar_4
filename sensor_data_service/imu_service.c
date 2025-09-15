
#include "imu_service.h"
#include "lsm6dsv.h"
// #include "sl_sleeptimer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h" // FreeRTOS V2 API

#include "common.h"


static imu_output_cb_t callback; // callback for sampled data
static int trig_cnt;

// static imu_operation_mode_t imu_operation_mode = IMU_INTERRUPT_MODE;
static int imu_sampling(void);

TaskHandle_t imu_task_handle = NULL;
volatile bool imu_task_running = false;
uint32_t imu_sample_period_ms = 0;

static int imu_sampling(void)
{
	imu_sample_t sample = {0};

	int16_t accelerometer[3]; // 单位：mg
  	int32_t gyroscope[3];     // 单位：mdps
	sample.timestamp =  get_unix_timestamp();
	sample.sample_count = trig_cnt;

    lsm6dsv_readdata(accelerometer, gyroscope);
    sample.ax = accelerometer[0];
    sample.ay = accelerometer[1];
    sample.az = accelerometer[2];
    sample.gx = gyroscope[0];
    sample.gy = gyroscope[1];
    sample.gz = gyroscope[2];

	if (callback != NULL) {
		callback(sample);
	}

	trig_cnt++;
	// app_log_debug("sample count: %d\r\n", trig_cnt);
	// app_log_debug("ax: %d, ay: %d, az: %d, gx: %d, gy: %d, gz: %d\r\n", sample.ax, sample.ay, sample.az, sample.gx, sample.gy, sample.gz);
	return 0;
}

static void imu_sampling_task(void *argument)
{
	UNUSED_PARAMETER(argument);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (imu_task_running) {
        imu_sampling();
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(imu_sample_period_ms));
    }
    
    // 任务退出前清理
	app_log_debug("delete IMU sampling task!\r\n");
    trig_cnt = 0;
    callback = NULL;
    imu_task_handle = NULL;
    vTaskDelete(NULL);
}


int imu_enable(output_data_rate_t rate, imu_output_cb_t cb)
{

	if (rate == IMU_ODR_0_HZ) {
		if (imu_task_handle != NULL){
			imu_task_running = false;

			// 等待任务安全退出（最多等待2个采样周期）
			uint32_t wait_time = imu_sample_period_ms * 2;
			if (wait_time < 10) wait_time = 10;
			
			osDelay(pdMS_TO_TICKS(wait_time));
			
			// 如果任务还未退出，强制删除
			if (imu_task_handle != NULL) {
				app_log_debug("Force delete IMU sampling task!\r\n");
				vTaskDelete(imu_task_handle);
				imu_task_handle = NULL;
			}
		}
		callback = NULL;
	}else {
		// 如果任务已在运行，先停止
		if (imu_task_handle != NULL) {
			imu_task_running = false;
			osDelay(pdMS_TO_TICKS(10));
		}
		
		callback = cb;
		imu_sample_period_ms = 1000 / rate;
		trig_cnt = 0; // clear count when enabling with a non-zero rate
	
		imu_task_running = true;
		BaseType_t status = xTaskCreate(
			imu_sampling_task,
			"IMUSampling",
			configMINIMAL_STACK_SIZE + 256,  // 适当增加栈大小
			NULL,
			osPriorityNormal2,            // 中等优先级
			&imu_task_handle
		);
		
		if (status != pdPASS) {
			app_log_error("Failed to create IMU sampling task!");
			imu_task_running = false;
			return -1;
		}
	}
    
    return 0;
}

int imu_get_trigger_count(void)
{
	return trig_cnt;
}

