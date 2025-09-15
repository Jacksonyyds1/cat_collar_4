// #include <stdio.h>
#include "sl_constants.h"
#include "cmsis_os2.h"

#include "storage_api.h"
#include "sl_sleeptimer.h"

#include "chekr_record.h"
#include "imu_service.h"

#include "ble_data_parse.h"
#include "ble_app_cat.h"
#include "common.h"
#include "storage_api.h"
#include "app_log.h"

static cont_rec_session_details_t cont_rec_session_details;

static file_handle_t m_handle;

char ses_uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator
uint8_t start_ble_live_imu_raw_flag = 0;
// 2 charactes for one byte,
// 8 x 2 bytes of unique ID of continuous recording, 1 x 2 byte for chunk ID, 1 byte for null terminator
static char cont_rec_uid_str[9 * 2 + 1] = {0}; 

// uint8_t start_read_chunk_res_flag = 0;
// uint8_t start_read_act_chunk_res_flag = 0;
uint16_t read_chunk_id = 0;
// uint8_t del_con_rec_ses_flag = 0;
// uint8_t start_cont_rec_after_flash_erase_flag = 0;
uint8_t start_rec_after_flash_erase_flag = 0;

// Global variables required for continuous recording
uint8_t continuous_recording_flag = 0;	// 0 - No continuous recording, 1 - Continuous recording

void sensor_data_record_init()
{
    for (int i = 0;i < 8;i++)
	{
		cont_rec_session_details.uid[i] = 0;
	}
    cont_rec_session_details.status = CONT_REC_RESV;
    cont_rec_session_details.rec_type = 0;
    cont_rec_session_details.chunk_size = 0;
    cont_rec_session_details.chunk_count = 0;
}

static void record_samples_cb(imu_sample_t data)
{
	// // put imu data on a work queue and process outside of trigger callback
	// struct sample_queue_t *sample_q = malloc(sizeof(struct sample_queue_t));
	// if (!sample_q) {
	// 	app_log_error("can't malloc sample_q!");
	// 	return -1;
	// }
	// memcpy((uint8_t *)&sample_q->sample, (uint8_t *)&data, sizeof(imu_sample_t));
	// k_work_init(&sample_q->work, record_sample_work_handler);
	// k_work_submit(&sample_q->work);
	
	// now process it for recording
	chekr_record_samples(data);
}
// used from the shell for testing
int chekr_set_filehandle(file_handle_t handle)
{
	m_handle = handle;
	return 0;
}

int chekr_record_samples(imu_sample_t data)
{
	static raw_imu_record_t record;
	static uint32_t record_count;
	uint8_t index = data.sample_count % RAW_FRAMES_PER_RECORD;

	// app_log_info("sample_count: %d, index: %d\r\n", data.sample_count, index);

	if (data.sample_count == 0) {
		// reset record_count on first sample of recording sessions
		record_count = 0;
	}

	if (index == 0) {
		// save timestamp of first sample in record
		record.timestamp = sys_cpu_to_be64(data.timestamp);
	}

	record.raw_data[index].ax = data.ax;
	record.raw_data[index].ay = data.ay;
	record.raw_data[index].az = data.az;
	record.raw_data[index].gx = data.gx;
	record.raw_data[index].gy = data.gy;
	record.raw_data[index].gz = data.gz;

	// write to flash when we fill in last sample in record
	if ((index + 1) % (RAW_FRAMES_PER_RECORD) == 0) {
		// record.record_num = sys_cpu_to_be32(record_count);
		record.record_num = (record_count);
		record_count++;

		// // BLE live broadcast the record if the flag is set
		// // else, write to flash.
		// if (1 == start_ble_live_imu_raw_flag)
		// {
		// 	// Notify the central device with the record data
		// 	typedef struct __attribute__((__packed__)) {
		// 		uint8_t soh;
		// 		raw_imu_record_t record;
		// 		uint16_t crc;
		// 	} start_ble_live_imu_raw_resp_t1;
		
		// 	start_ble_live_imu_raw_resp_t1 resp = { .soh = 2 };

		// 	memcpy(&resp.record, &record, sizeof(record));
		// 	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
		// 	notify_tocentral((uint8_t *)&resp, sizeof(resp));

		// } else {
			app_log_info("writing imu sample: %d\r\n", record.record_num);
			int ret = storage_write_raw_imu_record(m_handle, record);
			if (ret) {
				app_log_error("failed to write imu sample: ret=%d\r\n", ret);
				return -1;			
			}
		// }
	}

	// indication we're recording
	if (data.sample_count % BLIP_SAMPLE_INTERVAL == 0) {
		// pmic_blip_blue_led();
	}

	return 0;
}

static bool recording_in_progress(void)
{
	return m_handle != NULL;
}

// start recording to filename
int record_to_file(char *filename, RecordTypeE type)
{
	int ret;
	file_handle_t handle;

	if (type == RECORD_START) {
		handle = storage_open_file(filename);

		app_log_debug("handle: %p\r\n", handle);

		if (handle == NULL) {
			app_log_error("failed to open file %s\r\n", filename);
			return -1;
		}

		// set handle if we opened file successfully
		m_handle = handle;

		ret = imu_enable(RECORD_SAMPLE_RATE, record_samples_cb);
		if (ret) {
			app_log_error("failed to enable IMU\r\n");
			storage_close_file(m_handle);
			return -1;
		}

		// k_timer_start(&recording_timer, K_SECONDS(RECORDING_MAX_TIME_S), K_NO_WAIT);

		app_log_info("started IMU and writing IMU samples to file:%s\r\n", filename);
	} else {	// stop
		app_log_info("stopping IMU...\r\n");

		// k_timer_stop(&recording_timer);

		imu_enable(IMU_ODR_0_HZ, NULL); // stop IMU

		app_log_info("stopped IMU\r\n");

		ret = storage_close_file(m_handle);
		if (ret) {
			app_log_error("failed to close file %s\r\n", filename);
			return -1;
		}

		// pmic_blue_led_off();

		m_handle = NULL;
	}

	return 0;
}


/***********************************************/
// Checkr API jump table functions
/***********************************************/
int start_stop_rec_session(char *data, int len)
{
	(void)len;
	int ret = 0;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t start_stop;
		uint8_t uid[8];
	} start_stop_rec_session_req_t;

	start_stop_rec_session_req_t *req = (start_stop_rec_session_req_t *)data;

	// turn uid into a usable filename
	//char uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator

	for (int i = 0; i < 8; i++) {
		sprintf(&ses_uid_str[i * 2], "%02X", req->uid[i]);
	}

	if (req->start_stop && recording_in_progress()) {
		app_log_warning("stopping current recording, and starting a new one!");
		// filename is only used for logging when stopping the recording, the handle is
		// used internally for closing it via the storage module.
		record_to_file("active", RECORD_STOP);

#ifndef CHEKR_CONT_REC
		// Stop if there is a contiuous recording is in progress.
		if (continuous_recording_flag) {
			app_log_warning("stopping continuous recording!");
			continuous_recording_flag = 0;
			cont_rec_session_details.status = CONT_REC_STOPPED;			
		}		
#endif
	}

	if (req->start_stop)
	{
		start_rec_after_flash_erase_flag = 1;
	}
	else {
		ret = record_to_file(ses_uid_str, req->start_stop);	// Can stop directly.
	}	

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} start_stop_rec_session_resp_t;

	start_stop_rec_session_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(start_stop_rec_session_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = START_STOP_RECORDING_SESSION,
	};

	if (0 == ret) {
		resp.ack = ERR_NONE;
	} else {
		resp.ack = ERR_SLAVE_DEVICE_FAILURE;
	}

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

int read_rec_session_details_raw_imu(char *data, int len)
{
	(void)len;
	error_code_t ret = ERR_NONE;

	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t uid[8];
		uint8_t reserved;
	} read_rec_session_details_req_t;

	read_rec_session_details_req_t *req = (read_rec_session_details_req_t *)data;

	// turn uid into a usable filename
	char uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator

	for (int i = 0; i < 8; i++) {
		sprintf(&uid_str[i * 2], "%02X", req->uid[i]);
	}

	if (recording_in_progress()) {
		// stop a recording if we try to retrieve session details while recording in
		// progress
		app_log_warning("stopping current recording!");
		record_to_file("active", RECORD_STOP);
	}

	int16_t record_count = storage_get_raw_imu_record_count(uid_str);
	if (record_count < 0) {
		ret = ERR_NEGATIVE_ACK;
		record_count = 0;
	}

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t uid[8];
		uint16_t num_records;
		uint16_t crc;
	} read_rec_session_details_resp_t;

	read_rec_session_details_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(read_rec_session_details_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = READ_REC_SESSION_DETAILS_RAW_IMU,
	};

	memcpy(resp.uid, req->uid, sizeof(req->uid));
	resp.num_records = sys_cpu_to_be16(record_count);

	resp.ack = ret;

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

int read_rec_session_data_raw_imu(char *data, int len)
{
	(void)data;
	(void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t uid[8];
		uint8_t reserved[3];
		uint16_t record_num;
	} read_rec_session_data_req_t;

	read_rec_session_data_req_t *req = (read_rec_session_data_req_t *)data;

	uint16_t record_num = sys_be16_to_cpu(req->record_num);

	// turn uid into a usable filename
	char uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator

	for (int i = 0; i < 8; i++) {
		sprintf(&uid_str[i * 2], "%02X", req->uid[i]);
	}

	app_log_debug("reading record %d from file: %s", record_num, (char *)uid_str);
	activity_record_t record = {0};
	int ret = storage_read_activity_record((char *)uid_str, record_num, &record);
	if (ret < 0) {
		app_log_error("failed to read record %d from file: %s", record_num, (char *)uid_str);
		return -1;
	}

	// send response
	typedef struct __attribute__((__packed__)) {
		// NOTE: unusual header, only a SOH byte
		uint8_t soh;
		activity_record_t record;
		uint16_t crc;
	} read_rec_session_data_resp_t;

	read_rec_session_data_resp_t resp = {
		.soh = 2,
	};
	memcpy(&resp.record, &record, sizeof(record));

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

int read_rec_session_details_activity(char *data, int len)
{
	(void)len;
	error_code_t ret = ERR_NONE;

	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t uid[8];
		uint8_t reserved;
	} read_rec_session_details_req_t;

	read_rec_session_details_req_t *req = (read_rec_session_details_req_t *)data;

	// turn uid into a usable filename
	char uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator

	for (int i = 0; i < 8; i++) {
		sprintf(&uid_str[i * 2], "%02X", req->uid[i]);
	}

	if (recording_in_progress()) {
		// stop a recording if we try to retrieve session details while recording in
		// progress
		app_log_warning("stopping current recording!");
		record_to_file("active", RECORD_STOP);
	}

	int16_t record_count = storage_get_activity_record_count(uid_str);
	if (record_count < 0) {
		ret = ERR_NEGATIVE_ACK;
		record_count = 0;
	}

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t uid[8];
		uint16_t num_records;
		uint16_t crc;
	} read_rec_session_details_resp_t;

	read_rec_session_details_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(read_rec_session_details_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = READ_REC_SESSION_DETAILS_ACTIVITY,
	};

	memcpy(resp.uid, req->uid, sizeof(req->uid));
	resp.num_records = sys_cpu_to_be16(record_count);

	resp.ack = ret;

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

int read_rec_session_data_activity(char *data, int len)
{
	(void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t uid[8];
		uint8_t reserved[3];
		uint16_t record_num;
	} read_rec_session_data_req_t;

	read_rec_session_data_req_t *req = (read_rec_session_data_req_t *)data;

	uint16_t record_num = sys_be16_to_cpu(req->record_num);

	// turn uid into a usable filename
	char uid_str[8 * 2 + 1] = {0}; // 2 characters per byte + 1 for null terminator

	for (int i = 0; i < 8; i++) {
		sprintf(&uid_str[i * 2], "%02X", req->uid[i]);
	}

	app_log_debug("reading record %d from file: %s", record_num, (char *)uid_str);
	activity_record_t record = {0};
	int ret = storage_read_activity_record((char *)uid_str, record_num, &record);
	if (ret < 0) {
		app_log_error("failed to read record %d from file: %s", record_num, (char *)uid_str);
		return -1;
	}

	// send response
	typedef struct __attribute__((__packed__)) {
		// NOTE: unusual header, only a SOH byte
		uint8_t soh;
		activity_record_t record;
		uint16_t crc;
	} read_rec_session_data_resp_t;

	read_rec_session_data_resp_t resp = {
		.soh = 2,
	};
	memcpy(&resp.record, &record, sizeof(record));

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

void start_rec_after_flash_erase()
{
	delete_all_files();
	int ret = record_to_file(ses_uid_str, RECORD_START);

	if(ret == 0)
	{
		app_log_info("Regular recording started successfully after flash erase!");
	} else {
		app_log_error("Failed to start recording after flash erase!");

		// Reset the cont_rec_session_details structure.
		for (int i = 0;i < 8;i++)
		{
			ses_uid_str[i] = 0;
		}
	}
}




// // Function to send the response to the Read Activity Data Chunk response independent to command work queue.
// void send_read_act_chunk_res(void)
// {
// 	char fname[128];

// 	// Copy the UID into a string
// 	for (int i = 0; i < 8; i++) {
// 		sprintf(&fname[i * 2], "%02X", cont_rec_session_details.uid[i]);
// 	}
// 	// Add the chunk ID to the string
// 	uint8_t chunk_id = (uint8_t)read_chunk_id; 
// 	sprintf(&fname[8 * 2], "%02X", chunk_id);
	
// 	typedef struct __attribute__((__packed__)) {
// 		uint8_t soh;
// 		activity_record_t record;
// 		uint16_t crc;
// 	} get_cont_rec_session_status_resp_t1;

// 	get_cont_rec_session_status_resp_t1 resp = { .soh = 2 };
	
// 	int rnum = 0;	
// 	for (int i = 0;i < 30; i++)	// 30 seconds, 600 records.
// 	{
// 		for (int j = 0;j < 20;j++) // 20 * 50 = 1000 ms, 50ms for each record to send
// 		{
// 			activity_record_t record = {0};
// 			int ret = storage_read_activity_record((char *)fname, rnum, &record);

// 			if (ret < 0) {
// 				app_log_error("failed to read record %d from file: %s", rnum, (char *)fname);
// 				resp.record.record_num = 0xFFFF;
// 				//return;				
// 			}
// 			rnum++;
// 			memcpy(&resp.record, &record, sizeof(record));
// 			resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
// 			notify_tocentral((uint8_t *)&resp, sizeof(resp));
// 			//TODO: Add sleep 
// 			// sleep for 50ms
// 			// k_sleep(K_MSEC(45));
// 		}
// 		//TODO: Add watchdog feed here.
// 		// watchdog_feed();		// Feed the watchdog timer every 1 second.
// 	}			
// }

// void delete_cont_rec_session(void)
// {
// 	char fname[128];

// 	// Copy the UID into a string
// 	for (int i = 0; i < 8; i++) {
// 		sprintf(&fname[i * 2], "%02X", cont_rec_session_details.uid[i]);
// 	}
// 	// Iterate for all chunk IDs that are less than the current chunk ID, and delete them.
// 	for (uint8_t chunk_id = 1; chunk_id <= cont_rec_session_details.chunk_count; chunk_id++) {
// 		// Add the chunk ID to the string
// 		sprintf(&fname[8 * 2], "%02X", chunk_id);
		
// 		// Delete the continuous recording session chunk.
// 		if (storage_delete_cont_rec_chunk_file(fname) == 0) {
// 			app_log_info("Continuous recording session chunk %d deleted successfully!", chunk_id);
// 		} else {
// 			app_log_error("Failed to delete continuous recording session chunk %d!", chunk_id);
// 		}
// 	}

// 	// 2. Reset the cont_rec_session_details structure.
// 	for (int i = 0;i < 8;i++) {
// 		cont_rec_session_details.uid[i] = 0;
// 	}
// 	cont_rec_session_details.status = 0;
// 	cont_rec_session_details.rec_type = 0;
// 	cont_rec_session_details.chunk_size = 0;
// 	cont_rec_session_details.chunk_count = 0;

// 	//cmd_storage_erase();
// }


void start_cont_rec_after_flash_erase()
{
	delete_all_files();
	int ret = record_to_file(cont_rec_uid_str, RECORD_START);

	if(ret == 0){
		app_log_info("Continuous recording started successfully after flash erase!");
	} else {
		app_log_error("Failed to start continuous recording after flash erase!");

		// Reset the cont_rec_session_details structure.
		for (int i = 0;i < 8;i++)
		{
			cont_rec_session_details.uid[i] = 0;
		}
		cont_rec_session_details.status = 0;
		cont_rec_session_details.rec_type = 0;
		cont_rec_session_details.chunk_size = 0;
		cont_rec_session_details.chunk_count = 0;
		
		continuous_recording_flag = 0;
	}
}



///==============================================================================
///==============================================================================
void test_read_recorded_data(void)
{
    // // 1. 获取记录数量
    // int record_count = storage_get_raw_imu_record_count("active");
    // if (record_count <= 0) {
    //     app_log_error("No records found or error: %d\r\n", record_count);
    //     return;
    // }
    // app_log_info("Found %d IMU records in file\r\n", record_count);
    
    // // 2. 验证时间戳连续性
    // uint32_t last_timestamp = 0;
    // uint32_t valid_records = 0;
    
    // for (int i = 0; i < record_count; i++) {
    //     activity_record_t record;
    //     if (storage_read_activity_record("active", i, &record) != 0) {
    //         app_log_error("Failed to read record %d\r\n", i);
    //         continue;
    //     }

    //     // 验证时间戳连续性
    //     if (last_timestamp != 0) {
    //         uint32_t time_diff = record.timestamp - last_timestamp;
    //         if (time_diff < 90 || time_diff > 110) { // 预期约100ms间隔
    //             app_log_warning("Unexpected time gap at record %d: %d ms\r\n", 
    //                            i, time_diff);
    //         }
    //     }
    //     last_timestamp = record.timestamp;
        
    //     // 3. 验证记录号连续性
    //     if (record.record_num != (size_t)i) {
    //         app_log_warning("Record number mismatch: expected %d, got %d\r\n", 
    //                        i, record.record_num);
    //     }
        
    //     // 4. 验证传感器数据范围
    //     int valid_samples = 0;
    //     for (int j = 0; j < RAW_FRAMES_PER_RECORD; j++) {
    //         // 检查加速度计数据在合理范围内 (±16g)
    //         if (abs(record.raw_data[j].ax) > 16000 || 
    //             abs(record.raw_data[j].ay) > 16000 ||
    //             abs(record.raw_data[j].az) > 16000) {
    //             app_log_warning("Abnormal accel values in record %d, sample %d\r\n", i, j);
    //         }
            
    //         // 检查陀螺仪数据在合理范围内 (±2000dps)
    //         if (abs(record.raw_data[j].gx) > 200000 || 
    //             abs(record.raw_data[j].gy) > 200000 ||
    //             abs(record.raw_data[j].gz) > 200000) {
    //             app_log_warning("Abnormal gyro values in record %d, sample %d\r\n", i, j);
    //         }
            
    //         valid_samples++;
    //     }
        
    //     if (valid_samples == RAW_FRAMES_PER_RECORD) {
    //         valid_records++;
    //     }
        
    //     // 打印第一条记录的详细信息
    //     if (i == 0) {
    //         app_log_info("First record details:\r\n");
    //         app_log_info("  Timestamp: %lu\r\n", record.timestamp);
    //         app_log_info("  Record num: %d\r\n", record.record_num);
    //         app_log_info("  Sample 0: ax=%d, ay=%d, az=%d, gx=%d, gy=%d, gz=%d\r\n",
    //                      record.raw_data[0].ax, record.raw_data[0].ay, record.raw_data[0].az,
    //                      record.raw_data[0].gx, record.raw_data[0].gy, record.raw_data[0].gz);
    //     }
	//     // 打印第一条记录的详细信息
    //     if (i == 3) {
    //         app_log_info("Third record details:\r\n");
    //         app_log_info("  Timestamp: %lu\r\n", record.timestamp);
    //         app_log_info("  Record num: %d\r\n", record.record_num);
    //         app_log_info("  Sample 0: ax=%d, ay=%d, az=%d, gx=%d, gy=%d, gz=%d\r\n",
    //                      record.raw_data[0].ax, record.raw_data[0].ay, record.raw_data[0].az,
    //                      record.raw_data[0].gx, record.raw_data[0].gy, record.raw_data[0].gz);
    //     }
    // }
    
    // // 5. 汇总测试结果
    // app_log_info("===== TEST RESULTS =====\r\n");
    // app_log_info("Total records: %d\r\n", record_count);
    // app_log_info("Valid records: %d (%.1f%%)\r\n", 
    //             valid_records, (float)valid_records/record_count*100);
    // app_log_info("========================\r\n");
}

void cleanup_test_files(void)
{
	if (storage_get_raw_imu_record_count("active") > 0) 
	{
	  char filepath[MAX_PATH_LEN];
	  snprintf(filepath, sizeof(filepath), "%s/%s.imu", fs_mount_cat.mnt_point, "active");
	  if (fs_unlink(filepath) == 0) {
		app_log_info("Test file deleted successfully\r\n");
	  } else {
		app_log_error("Failed to delete test file\r\n");
	  }
	}
}

/*
void test_file_close() 
{
    fs_file_t test_file;
    fs_file_t_init(&test_file);
    
    // 创建小文件
    int ret = fs_open(&test_file, "/cat/test.close", FS_O_CREATE | FS_O_WRITE);
    if(ret == 0) {
        const char data[] = "test data";
        fs_write(&test_file, data, sizeof(data));
        
        // 立即关闭
        app_log_debug("Testing file close...\r\n");
        fs_close(&test_file);
        app_log_debug("File closed successfully\r\n");
    }
}
*/

