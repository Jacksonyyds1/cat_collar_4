#ifndef _CHEKR_RECORD_H
#define _CHEKR_RECORD_H

#include "stdint.h"
#include "imu_service.h"

#define RECORD_SAMPLE_RATE (IMU_ODR_15_HZ)

typedef enum {
	RECORD_STOP = 0,
	RECORD_START,
} RecordTypeE;

typedef enum {
	CONT_REC_RESV = 0,
	CONT_REC_IN_PROGRESS,
	CONT_REC_STOPPED,
	CONT_REC_STOPPED_MEM_FAULT,
	CONT_REC_NOT_FOUND = 0xFF
} ContRecStatusE;

// Static variables required for continuous recording
typedef struct {
	uint8_t uid[8];
	uint8_t status;			// 0 - Reserved, 1 - In progress, 2 - Stopped, 3 - Stopped due to memory fault
	uint8_t rec_type;		// static 0x01 for now, 10 minutes per chunk
	uint16_t chunk_size;	// fixed length for now, 10 minutes, 900 records
	uint16_t chunk_count;	// Number of chunks in the continuous recording session
} cont_rec_session_details_t;


void sensor_data_record_init();

// void delete_cont_rec_session(void);

/// test
int record_to_file(char *filename, RecordTypeE type);
void test_read_recorded_data(void);
void cleanup_test_files(void);
// void test_file_close();

/// public API
int chekr_record_samples(imu_sample_t data);


// recording related jump table functions in chekr_record module
int start_stop_rec_session(char *data, int len);
int read_rec_session_details_raw_imu(char *data, int len);
int read_rec_session_data_raw_imu(char *data, int len);
int read_rec_session_details_activity(char *data, int len);
int read_rec_session_data_activity(char *data, int len);

#endif // _CHEKR_RECORD_H