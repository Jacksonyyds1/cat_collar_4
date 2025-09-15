// #include "FreeRTOS.h"
#include "ble_data_parse.h"
#include "cmsis_os2.h"
#include "time.h"
// #include "sys.h"
// #include "task.h"
// #include "queue.h"
#include "app_log.h"
#include "common.h"
#include "ble_app_cat.h"
#include "chekr_dash.h"
#include "chekr_record.h"
#include "rsi_bt_common_apis.h"

typedef int (*command_funcp_t)(char *data, int len);

typedef struct {
	commands_list_t command;
	command_funcp_t func;
	uint8_t frame_len;
} cmd_jump_entry_t;


// command declarations
static int get_device_mac_addr(char *data, int len);
static int get_device_system_info(char *data, int len);

// TODO: remove unused attribute after included in jump table
static int get_user_info(char *data, int len) __attribute__((unused));
static int set_dog_collar_position(char *data, int len);
static int set_dog_size(char *data, int len);
static int get_epoch_rtc(char *data, int len);
static int set_epoch_rtc(char *data, int len);
static int start_stop_raw_data_harvesting(char *data, int len) __attribute__((unused));
static int start_stop_activity_data_harvesting(char *data, int len) __attribute__((unused));
static int start_stop_periodic_dashboard_status_info(char *data, int len);
static int ble_status_show(char *data, int len);
static int ble_connected_show(char *data, int len);
static int factory_reset(char *data, int len) __attribute__((unused));
static int reboot(char *data, int len);
static int system_alarm_status_notif(char *data, int len) __attribute__((unused));
static int system_general_notif(char *data, int len) __attribute__((unused));
static int read_raw_rec_session_data(char *data, int len) __attribute__((unused));
static int read_activity_rec_session_data(char *data, int len) __attribute__((unused));
static int read_device_log_info(char *data, int len) __attribute__((unused));
static int read_dashboard_info(char *data, int len);

// command jump table
static cmd_jump_entry_t command_jump_table[] = {

	// these commands are called after connection
	{SET_EPOCH_RTC, set_epoch_rtc, SET_EPOCH_RTC_FRAME_LEN},
	{GET_EPOCH_RTC, get_epoch_rtc, GET_EPOCH_RTC_FRAME_LEN},
	{SET_DOG_COLLAR_POSITION, set_dog_collar_position, SET_DOG_COLLAR_POS_FRAME_LEN},
	{SET_DOG_SIZE, set_dog_size, SET_DOG_SIZE_FRAME_LEN},
	{READ_DASHBOARD_INFO, read_dashboard_info, READ_DASHBOARD_INFO_FRAME_LEN},

	// these commands are used for a typical raw data recording session
	{START_STOP_RECORDING_SESSION, start_stop_rec_session, START_STOP_REC_SESSION_FRAME_LEN},
	{READ_REC_SESSION_DETAILS_RAW_IMU, read_rec_session_details_raw_imu, READ_REC_SESSION_DETAILS_FRAME_LEN},
	{READ_REC_SESSION_DATA_RAW_IMU, read_rec_session_data_raw_imu, READ_REC_SESSION_DATA_FRAME_LEN},
	{READ_REC_SESSION_DETAILS_ACTIVITY, read_rec_session_details_activity, READ_REC_SESSION_DETAILS_FRAME_LEN},
	{READ_REC_SESSION_DATA_ACTIVITY, read_rec_session_data_activity, READ_REC_SESSION_DATA_FRAME_LEN},

	// unsure if and when these are used
	{GET_DEVICE_MAC_ADDR, get_device_mac_addr, GET_DEVICE_MAC_ADDR_FRAME_LEN},
	{GET_DEVICE_SYSTEM_INFO, get_device_system_info, GET_DEVICE_SYSTEM_INFO_FRAME_LEN},
	{CAT_FACTORY_RESET, factory_reset, FACTORY_RESET_FRAME_LEN},
	{REBOOT, reboot, REBOOT_FRAME_LEN},
	{BLE_STATUS_SHOW, ble_status_show, BLE_STATUS_SHOW_FRAME_LEN},
	{BLE_CONNECTED_SHOW, ble_connected_show, BLE_CONNECTED_SHOW_FRAME_LEN},
	{START_STOP_PERIODIC_DASHBOARD_STATUS_INFO, start_stop_periodic_dashboard_status_info, START_STOP_PERIODIC_DASH_FRAME_LEN},

	// // fill in more commands as necessary here
	// {START_BLE_LIVE_RAW_IMU_RECORDING, start_ble_live_rec_session, START_STOP_BLE_LIVE_RAW_IMU_FRAME_LEN},
	// // CHEKR_CONT_REC
	// {START_STOP_RAW_DATA_HARVESTING, start_stop_cont_rec_session, START_STOP_RAW_DATA_HARVESTING_FRAME_LEN},
	// {GET_CONT_REC_SESSION_STATUS, get_cont_rec_session_status, GET_CONT_REC_SESSION_STATUS_FRAME_LEN},
	// {READ_CONT_REC_SESSION_DETAILS, read_cont_rec_session_details, READ_CONT_REC_SESSION_DETAILS_FRAME_LEN},
	// {READ_RAW_DATA_CHUNK, read_raw_data_chunk, READ_RAW_DATA_CHUNK_FRAME_LEN},
	// {DELETE_RAW_DATA_CHUNK, delete_raw_data_chunk, DELETE_RAW_DATA_CHUNK_FRAME_LEN},
	// {DELETE_REC_SESSION, delete_recording_session, DELETE_REC_SESSION_FRAME_LEN},
	// {READ_ACTIVITY_DATA_CHUNK, read_activity_data_chunk, READ_ACTIVITY_DATA_CHUNK_FRAME_LEN},
	// // CHEKR_CONT_REC
};

// commands
// TODO: split these out into separate files based on functionality
static int get_device_mac_addr(char *data, int len)
{
    (void)data;
    (void)len;
	// size_t count = 1;
	// bt_addr_le_t addr = {0};
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t mac_addr[6];
		uint16_t crc;
	} get_mac_addr_resp_t;

	get_mac_addr_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(get_mac_addr_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = GET_DEVICE_MAC_ADDR,
		resp.ack = ERR_NONE,
	};

    uint8_t bt_addr[6];
    rsi_bt_get_local_device_address(bt_addr);
    // app_log_info("Local Device Address from module: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
    //             local_device_addr[0], local_device_addr[1], local_device_addr[2],
    //             local_device_addr[3], local_device_addr[4], local_device_addr[5]);
    memcpy(resp.mac_addr, bt_addr, sizeof(resp.mac_addr));
	// resp.mac_addr[0] = bt_addr[0];
    // resp.mac_addr[1] = bt_addr[1];
    // resp.mac_addr[2] = bt_addr[2];
    // resp.mac_addr[3] = bt_addr[3];
    // resp.mac_addr[4] = bt_addr[4];
    // resp.mac_addr[5] = bt_addr[5];

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));

	return 0;
}

static int get_device_system_info(char *data, int len)
{
	(void)data;
	(void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t boot_major;
		uint8_t boot_minor;
		uint8_t boot_patch;
		uint8_t app_fw_major;
		uint8_t app_fw_minor;
		uint8_t app_fw_patch;
		uint8_t pcba_major;
		uint8_t pcba_minor;
		uint8_t pcba_patch;
		uint8_t device_name[6];
		uint8_t device_model;
		uint8_t mfg_code;
		uint8_t factory_code;
		uint8_t mfg_date[4];
		uint8_t serial_num[8];
		uint8_t reserved[11];
		uint16_t crc;
	} get_device_info_resp_t;

	get_device_info_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(get_device_info_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = GET_DEVICE_SYSTEM_INFO,
		.ack = ERR_NONE,

		.app_fw_major = CATCOLLAR_APPLICATION_MAJOR_VERSION,
		.app_fw_minor = CATCOLLAR_APPLICATION_MINOR_VERSION,
		.app_fw_patch = CATCOLLAR_APPLICATION_PATCH_VERSION,

		.pcba_major = PCBA_MAJOR,
		.pcba_minor = PCBA_MINOR,
		.pcba_patch = PCBA_PATCH,

		.device_name = {'t', 'r', 'a', 'k', 'r', '1'},
		.device_model = DEVICE_MODEL,
		.mfg_code = MFG_CODE,
		.factory_code = FACTORY_CODE,
		.mfg_date = {0x65, 0x2f, 0x1b, 0xc9} // todays date - Big Endian?
	};
	// skip 'NDC' in name
	memcpy(resp.serial_num, ble_get_local_name() + 3, sizeof(resp.serial_num));

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));

	return 0;
}

static int get_user_info(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int set_dog_collar_position(char *data, int len)
{
	(void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t position;
		uint16_t crc;
	} set_dog_collar_pos_t;

	set_dog_collar_pos_t *pos = (set_dog_collar_pos_t *)data;
	app_log_debug("set position to %d\r\n", pos->position);

	// TODO: what do we do with position?

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} set_dog_collar_pos_resp_t;

	set_dog_collar_pos_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(set_dog_collar_pos_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = SET_DOG_COLLAR_POSITION,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

static int set_dog_size(char *data, int len)
{
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t size;
		uint16_t crc;
	} set_dog_size_t;

	set_dog_size_t *p = (set_dog_size_t *)data;
	pet_size_t size = p->size;
	//ml_set_dog_size(size);

	app_log_debug("set size to %d\r\n", size);

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint16_t crc;
	} set_dog_size_resp_t;

	set_dog_size_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(set_dog_size_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = SET_DOG_SIZE,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

static int get_epoch_rtc(char *data, int len)
{
    (void)data;
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t time[8];
		uint16_t crc;
	} get_epoch_time_resp_t;

	get_epoch_time_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(get_epoch_time_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = GET_EPOCH_RTC,
		.ack = ERR_NONE,
	};

	uint64_t currentmillis = sys_cpu_to_be64(get_unix_timestamp_ms());
	memcpy(resp.time, &currentmillis, sizeof(currentmillis));

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));

	return 0;
}
static int set_epoch_rtc(char *data, int len)
{
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint64_t time;
		uint16_t crc;
	} set_epoch_time_t;

	set_epoch_time_t *t = (set_epoch_time_t *)data;

    uint64_t currentmillis = sys_be64_to_cpu(t->time);

    // time_t seconds = currentmillis / 1000;
	// app_log_info("Current time in seconds: %ld\r\n", (long)seconds);
    // uint32_t milliseconds = currentmillis % 1000;
    // struct tm *timeinfo = gmtime(&seconds); // UTC 时间
    // if (timeinfo != NULL) {
    //     app_log_debug("UTC time: %04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
    //                  timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
    //                  timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, milliseconds);
    //     // 对于北京时间（UTC+8），添加时区偏移
    //     time_t beijing_seconds = seconds + 8 * 3600;
    //     struct tm *beijing_timeinfo = gmtime(&beijing_seconds); // 调整时区
    //     app_log_debug("Beijing time: %04d-%02d-%02d %02d:%02d:%02d.%03d\r\n",
    //                  beijing_timeinfo->tm_year + 1900, beijing_timeinfo->tm_mon + 1, beijing_timeinfo->tm_mday,
    //                  beijing_timeinfo->tm_hour, beijing_timeinfo->tm_min, beijing_timeinfo->tm_sec, milliseconds);
    // } else {
    //     app_log_error("Failed to convert time\r\n");
    // }


	// struct timespec ts;
	// ts.tv_sec = currentmillis / 1000;
	// ts.tv_nsec = (currentmillis % 1000) * 1000000;
	// int ret = clock_settime(CLOCK_REALTIME, &ts);
	// if (ret != 0) {
	// 	app_log_error("Failed to set system time: ret=%d\r\n", ret);
	// 	// FIXME: goto out with .ack ERR set
	// 	return -1;
	// } else {
	// 	app_log_debug("set clock to %lu\r\n", currentmillis);
	// }

	// 4. 设置系统时间
	set_unix_timestamp((uint32_t)(currentmillis / 1000));

	// send response
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} set_epoch_time_resp_t;

	set_epoch_time_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(set_epoch_time_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = SET_EPOCH_RTC,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
    return 0;
}


static int start_stop_raw_data_harvesting(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int start_stop_activity_data_harvesting(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int start_stop_periodic_dashboard_status_info(char *data, int len)
{
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		dashboard_ctrl_t start_stop_once;
		uint8_t interval;
		uint16_t crc;
	} ble_periodic_dashboard_set_t;

	ble_periodic_dashboard_set_t *dash = (ble_periodic_dashboard_set_t *)data;
	dashboard_ctrl(dash->start_stop_once, dash->interval);

	return 0;
}

// ble_status_show and ble_connected_show look idential from the spec
static int ble_status_show(char *data, int len)
{
    (void)data;
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} ble_status_show_resp_t;

	ble_status_show_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(ble_status_show_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = BLE_STATUS_SHOW,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

static int ble_connected_show(char *data, int len)
{
    (void)data;
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} ble_status_show_resp_t;

	ble_status_show_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(ble_status_show_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = BLE_CONNECTED_SHOW,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
	return 0;
}

static int factory_reset(char *data, int len)
{
    (void)data;
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} ble_factory_reset_resp_t;

	ble_factory_reset_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(ble_factory_reset_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = CAT_FACTORY_RESET,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));

	// TODO: what do we do for factory reset?
	return 0;
}

static int reboot(char *data, int len)
{
    (void)data;
    (void)len;
	typedef struct __attribute__((__packed__)) {
		frame_format_header_t header;
		uint8_t ack;
		uint8_t reserved;
		uint16_t crc;
	} ble_reboot_resp_t;

	ble_reboot_resp_t resp = {
		.header.start_byte = SB_DEVICE_TO_MOBILE_APP,
		.header.frame_len = sizeof(ble_reboot_resp_t),
		.header.frame_type = FT_REPORT,
		.header.cmd = REBOOT,
		.ack = ERR_NONE,
	};

	resp.crc = utils_crc16_modbus((const uint8_t *)&resp, sizeof(resp) - sizeof(uint16_t));
	write_to_central((uint8_t *)&resp, sizeof(resp));
    //TODO: reboot
	// sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

static int system_alarm_status_notif(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int system_general_notif(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int read_raw_rec_session_data(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int read_activity_rec_session_data(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int read_device_log_info(char *data, int len)
{
    (void)data;
    (void)len;
	return 0;
}

static int read_dashboard_info(char *data, int len)
{
    (void)data;
    (void)len;
	dashboard_response();
	return 0;
}

// #ifndef CHEKR_CONT_REC
void notify_tocentral(uint8_t *data, int len)
{
    (void)data;
    (void)len;
	// if (notify_enable) {
	// 	bt_gatt_notify(NULL, &chekr_service.attrs[1], data, len);
	// }
}
// #endif


///=======================================================================
///=======================================================================
QueueHandle_t bluetooth_data_queue = NULL;
static void command_parse_work_handler(void *params)
{
    (void)params;
    CommandBufferT command_buffer;
    while(1)
    {
        if(xQueueReceive(bluetooth_data_queue, (void *)&command_buffer, 10 / portTICK_RATE_MS) == pdPASS)
		{
            frame_format_header_t *header = (frame_format_header_t *)command_buffer.data;
            // some basic checks
            if (header->start_byte != SB_MOBILE_APP_TO_DEVICE) {
                app_log_error("invalid start byte: %02x\r\n", header->start_byte);
                // FIXME: report Invalid Start byte
                // return;
            }
            
            uint8_t frame_len = header->frame_len;

            if (frame_len != command_buffer.len) {
                app_log_error("frame length mismatch: received %d, expected %d\r\n", command_buffer.len, frame_len);
                // FIXME: report Length is not matching
                // return;
            }

            // TODO: verify crc endianess, verify it includes entire frame
            uint16_t req_crc = *(uint16_t *)(command_buffer.data + (command_buffer.len - 2));
            uint16_t calc_crc = utils_crc16_modbus(command_buffer.data, command_buffer.len - sizeof(uint16_t));
            if (calc_crc != req_crc) {
                app_log_error("invalid crc: request=%04x, calc=%04x\r\n", req_crc, calc_crc);
                // FIXME: report Invalid Checksum
                // return;
            }

            // app_log_info("processing command: %d\r\n", header->cmd);

            // jump to command if defined
            for (unsigned int i = 0; i < sizeof(command_jump_table) / sizeof(cmd_jump_entry_t); i++) {
                commands_list_t command = command_jump_table[i].command;
                command_funcp_t func = command_jump_table[i].func;
                uint8_t expected_frame_len = command_jump_table[i].frame_len;

                if (header->cmd == command && func) {

                    if (expected_frame_len != command_buffer.len) {
                        app_log_error("jump table frame length mismatch: received %d, expected ""%d\r\n",
                            command_buffer.len, expected_frame_len);
                        // FIXME: report Length is not matching
                        // return;
                    }

                    app_log_debug("calling func for command: %d\r\n", command);
                    func((char *)command_buffer.data, command_buffer.len);
                    // return;
                }
            }
            // app_log_warning("command not defined: %d\r\n", header->cmd);
            // FIXME: report Unrecognized Command
        }
    }
}

static bool enter_bt_parse_task_flag = false;
#define TASK_CMD_PARSE_DATA_STACK_SIZE   1024
static StackType_t ble_parse_data_TaskStack[TASK_CMD_PARSE_DATA_STACK_SIZE];
static StaticTask_t ble_parse_data_TaskHandle;
void ble_data_parse_task_init(void)
{
    if (enter_bt_parse_task_flag == false){
        enter_bt_parse_task_flag = true;
        bluetooth_data_queue = xQueueCreate(5, sizeof(CommandBufferT));
        if(bluetooth_data_queue == NULL) {
            app_log_error("Failed to create bluetooth_data_queue\r\n");
            return;
        }
        app_log_info("bluetooth_data_queue created successfully\r\n");

         if (xTaskCreateStatic(
            command_parse_work_handler,           // 任务函数
            "command_parse_work_handler",         // 任务名称
            TASK_CMD_PARSE_DATA_STACK_SIZE,       // 堆栈大小
            NULL,                                 // 任务参数
            osPriorityNormal3,                    // 任务优先级 configMAX_PRIORITIES
            ble_parse_data_TaskStack,      // 静态任务堆栈
            &ble_parse_data_TaskHandle     // 静态任务控制块
            ) == NULL)
        {
            app_log_error("Failed to create command_parse_work_handler\r\n");
            vQueueDelete(bluetooth_data_queue);
            return;
        }
        app_log_info("command_parse_work_handler created successfully\r\n");
    }
}

void ble_data_parse_task_delete(void)
{
	app_log_debug("ble_parse_task_delete\r\n");
	if (enter_bt_parse_task_flag == true){
		enter_bt_parse_task_flag = false;
		if (bluetooth_data_queue != NULL){
			vQueueDelete(bluetooth_data_queue);		 //删除蓝牙数据接收队列
		}
	}
}
