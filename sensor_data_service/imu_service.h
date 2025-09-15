#ifndef IMU_SERVICE_H
#define IMU_SERVICE_H

#include <stdint.h>

// typedef enum { // IMU operation mode
// 	IMU_POLL_MODE = 0,
// 	IMU_INTERRUPT_MODE
// } imu_operation_mode_t;

// note: we're currently not supporting 1.875 or 7.5 Hz rates
typedef enum {
	IMU_ODR_0_HZ,
	IMU_ODR_15_HZ = 15,
	IMU_ODR_30_HZ = 30,
	IMU_ODR_60_HZ = 60,
	IMU_ODR_120_HZ = 120,
	IMU_ODR_240_HZ = 240,
	IMU_ODR_480_HZ = 480,
	IMU_ODR_960_HZ = 960,
	IMU_ODR_1920_HZ = 1920,
	IMU_ODR_3840_HZ = 3840,
	IMU_ODR_7680_HZ = 7680,
	IMU_ODR_MAX_HZ,
} output_data_rate_t;

typedef struct __attribute__((__packed__)) {
	uint32_t timestamp;
	uint32_t sample_count;
	int16_t ax, ay, az; // accelerometer
	int32_t gx, gy, gz; // gyro
} imu_sample_t;

// typedef void imu_output_callback_function(imu_sample_t);
// typedef imu_output_callback_function* imu_output_cb_t;

typedef void (*imu_output_cb_t)(imu_sample_t output);


// extern TaskHandle_t imu_task_handle;
// extern volatile bool imu_task_running;
// extern uint32_t imu_sample_period_ms;

int imu_enable(output_data_rate_t rate, imu_output_cb_t callback);
int imu_get_trigger_count(void);

#endif /* IMU_SERVICE_H */
