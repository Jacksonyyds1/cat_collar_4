#ifndef __STORAGE_API_H__
#define __STORAGE_API_H__

#include <stdint.h>
#include "filesystem_port.h"

// 文件系统挂载点配置
#define MOUNT_POINT_CAT   "/cat"
#define PATH_CAT(...)     MOUNT_POINT_CAT"/"__VA_ARGS__

#define MAX_PATH_LEN (256)
#define MAX_OPEN_FILES (1)

#define RAW_FRAMES_PER_RECORD 	(10)
#define BLIP_SAMPLE_INTERVAL 	(50)

extern fs_mount_t fs_mount_cat;

// opaque type for handle, internal structure is hidden
// typedef struct file_data_t file_data_t;
typedef struct file_data_t *file_handle_t;

typedef struct __attribute__((__packed__)) {
	int16_t ax, ay, az; // accelerometer   2*3 = 6 bytes
	int32_t gx, gy, gz; // gyro			   4*3 = 12 bytes
} raw_imu_data_frame_t;

// ChekrAppLink API sends raw data as a 252-byte record,
// containing 10 samples per record
typedef struct __attribute__((__packed__)) {
	uint32_t record_num;
	uint32_t timestamp; // currentmillis
	raw_imu_data_frame_t raw_data[RAW_FRAMES_PER_RECORD];
} raw_imu_record_t;

typedef struct __attribute__((__packed__)) {
	uint8_t start_byte;
	uint8_t pred_class;
	float pred_probability;
	float pred_reps;
	uint32_t start_timestamp;
	uint32_t end_timestamp;
	float accel_mag_mean_val;
	float gyro_mag_mean_val;
	float accel_mag_sd_val;
	float gyro_mag_sd_val;
} activity_data_frame_t;

typedef struct __attribute__((__packed__)) {
	uint32_t record_num;
	activity_data_frame_t activity_data; // only one per frame
} activity_record_t;

int storage_init();
int delete_all_files();

// open a new file for writing.  returns file handle or NULL
file_handle_t storage_open_file(char *fname);
// close and save file
int storage_close_file(file_handle_t handle);

// raw IMU related interfaces
int storage_write_raw_imu_record(file_handle_t handle, raw_imu_record_t raw_record);
int storage_get_raw_imu_record_count(char *basename);

int storage_read_activity_record(char *basename, int record_number, activity_record_t *record);
int storage_get_activity_record_count(char *basename);

// CHEKR_CONT_REC
int storage_delete_cont_rec_chunk_file(char *basename);

#endif // __STORAGE_APP_H__