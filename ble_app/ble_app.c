/*******************************************************************************
* @file  ble_app.c
 * @brief : This file contains example application for WiFi Station BLE
 * Provisioning
 * @section Description :
 * This application explains how to get the WLAN connection functionality using
 * BLE provisioning.
 * Silicon Labs Module starts advertising and with BLE Provisioning the Access Point
 * details are fetched.
 * Silicon Labs device is configured as a WiFi station and connects to an Access Point.
 =================================================================================*/

//! SL Wi-Fi SDK includes
#include "sl_board_configuration.h"
#include "sl_constants.h"
#include "sl_wifi.h"
#include "sl_net_ip_types.h"
#include "cmsis_os2.h"
#include "sl_utility.h"

// BLE include file to refer BLE APIs
#include <rsi_ble_apis.h>
#include <rsi_bt_common_apis.h>
#include <rsi_common_apis.h>
#include <string.h>

#include "ble_config.h"
#include "wifi_config.h"
#include "ble_prov.h"

#include "app_log.h"
#include "ble_app_cat.h"
#include "ble_data_parse.h"
#include "blinky.h"
#include "stdio.h"

// #define RSI_BLE_PROVISIONING_ENABLED

// application event list
#define RSI_BLE_ENH_CONN_EVENT 0x01
#define RSI_BLE_DISCONN_EVENT  0x02
#define RSI_BLE_WLAN_SCAN_RESP 0x03

#define RSI_SSID                        0x0D
#define RSI_SECTYPE                     0x0E
#define RSI_BLE_WLAN_DISCONN_NOTIFY     0x0F
#define RSI_WLAN_ALREADY                0x10
#define RSI_WLAN_NOT_ALREADY            0x11
#define RSI_BLE_WLAN_TIMEOUT_NOTIFY     0x12
#define RSI_APP_FW_VERSION              0x13
#define RSI_BLE_WLAN_DISCONNECT_STATUS  0x14
#define RSI_BLE_WLAN_JOIN_STATUS        0x15
#define RSI_BLE_MTU_EVENT               0x16
#define RSI_BLE_CONN_UPDATE_EVENT       0x17
#define RSI_BLE_RECEIVE_REMOTE_FEATURES 0x18
#define RSI_BLE_DATA_LENGTH_CHANGE      0x19

// Maximum length of SSID
#define RSI_SSID_LEN 34
// MAC address length
#define RSI_MAC_ADDR_LEN 6
// Maximum Acccess points that can be scanned
#define RSI_AP_SCANNED_MAX 11

// global parameters list
static volatile uint32_t ble_app_event_map;
rsi_ble_event_conn_status_t conn_event_to_app;
static rsi_ble_event_disconnect_t disconn_event_to_app;
uint8_t coex_ssid[50];
uint8_t pwd[RSI_BLE_PROV_MAX_DATA_LEN];
uint8_t sec_type;
sl_wifi_scan_result_t *scanresult = NULL;

uint8_t remote_dev_addr[18] = { 0 };
static rsi_ble_event_mtu_t app_ble_mtu_event;
static rsi_ble_event_conn_update_t event_conn_update_complete;
static rsi_ble_event_remote_features_t remote_dev_feature;
// static rsi_ble_event_data_length_update_t updated_data_len_params;

extern uint8_t connected, disassosiated;
extern uint8_t retry;
extern sl_net_ip_configuration_t ip_address;
extern osSemaphoreId_t ble_thread_sem;
extern uint16_t scanbuf_size;

/******************************************************
 *               Function Declarations
 ******************************************************/
extern void wifi_app_set_event(uint32_t event_num);
void rsi_ble_on_enhance_conn_status_event(rsi_ble_event_enhance_conn_status_t *resp_enh_conn);
void rsi_ble_configurator_init(void);
void rsi_ble_configurator_task(void *argument);
/*==============================================*/
/**
 * @fn         rsi_ble_app_set_event
 * @brief      sets the specific event.
 * @param[in]  event_num, specific event number.
 * @return     none.
 * @section description
 * This function is used to set/raise the specific event.
 */
static void rsi_ble_app_set_event(uint32_t event_num)
{
  ble_app_event_map |= BIT(event_num);

  osSemaphoreRelease(ble_thread_sem);

  return;
}

/*==============================================*/
/**
 * @fn         rsi_ble_app_clear_event
 * @brief      clears the specific event.
 * @param[in]  event_num, specific event number.
 * @return     none.
 * @section description
 * This function is used to clear the specific event.
 */
static void rsi_ble_app_clear_event(uint32_t event_num)
{
  ble_app_event_map &= ~BIT(event_num);
  return;
}

/*==============================================*/
/**
 * @fn         rsi_ble_app_get_event
 * @brief      returns the first set event based on priority
 * @param[in]  none.
 * @return     int32_t
 *             > 0  = event number
 *             -1   = not received any event
 * @section description
 * This function returns the highest priority event among all the set events
 */
static int32_t rsi_ble_app_get_event(void)
{
  uint32_t ix;

  for (ix = 0; ix < 32; ix++) {
    if (ble_app_event_map & (1 << ix)) {
      return ix;
    }
  }

  return (-1);
}

/*==============================================*/
/**
 * @fn         rsi_ble_on_enhance_conn_status_event
 * @brief      invoked when enhanced connection complete event is received
 * @param[out] resp_enh_conn, connected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the status of the connection
 */
void rsi_ble_on_enhance_conn_status_event(rsi_ble_event_enhance_conn_status_t *resp_enh_conn)
{
  conn_event_to_app.dev_addr_type = resp_enh_conn->dev_addr_type;
  memcpy(conn_event_to_app.dev_addr, resp_enh_conn->dev_addr, RSI_DEV_ADDR_LEN);

  conn_event_to_app.status = resp_enh_conn->status;
  rsi_ble_app_set_event(RSI_BLE_ENH_CONN_EVENT);
}

/*==============================================*/
/**
 * @fn         rsi_ble_on_connect_event
 * @brief      invoked when connection complete event is received
 * @param[out] resp_conn, connected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the status of the connection
 */
static void rsi_ble_on_connect_event(rsi_ble_event_conn_status_t *resp_conn)
{
  memcpy(&conn_event_to_app, resp_conn, sizeof(rsi_ble_event_conn_status_t));

  rsi_ble_app_set_event(RSI_BLE_ENH_CONN_EVENT);
}

/*==============================================*/
/**
 * @fn         rsi_ble_on_disconnect_event
 * @brief      invoked when disconnection event is received
 * @param[out]  resp_disconnect, disconnected remote device information
 * @param[out]  reason, reason for disconnection.
 * @return     none.
 * @section description
 * This Callback function indicates disconnected device information and status
 */
static void rsi_ble_on_disconnect_event(rsi_ble_event_disconnect_t *resp_disconnect, uint16_t reason)
{
  UNUSED_PARAMETER(reason);
  memcpy(&disconn_event_to_app, resp_disconnect, sizeof(rsi_ble_event_disconnect_t));
  rsi_ble_app_set_event(RSI_BLE_DISCONN_EVENT);
}

/*==============================================*/
/**
 * @fn         rsi_ble_on_conn_update_complete_event
 * @brief      invoked when conn update complete event is received
 * @param[out] rsi_ble_event_conn_update_complete contains the controller
 * support conn information.
 * @param[out] resp_status contains the response status (Success or Error code)
 * @return     none.
 * @section description
 * This Callback function indicates the conn update complete event is received
 */
void rsi_ble_on_conn_update_complete_event(rsi_ble_event_conn_update_t *rsi_ble_event_conn_update_complete,
                                           uint16_t resp_status)
{
  UNUSED_PARAMETER(resp_status);
  rsi_6byte_dev_address_to_ascii(remote_dev_addr, (uint8_t *)rsi_ble_event_conn_update_complete->dev_addr);
  memcpy(&event_conn_update_complete, rsi_ble_event_conn_update_complete, sizeof(rsi_ble_event_conn_update_t));
  rsi_ble_app_set_event(RSI_BLE_CONN_UPDATE_EVENT);
}

/*============================================================================*/
/**
 * @fn         rsi_ble_on_remote_features_event
 * @brief      invoked when LE remote features event is received.
 * @param[out] rsi_ble_event_remote_features, connected remote device information
 * @return     none.
 * @section description
 * This callback function indicates the remote device features
 */
void rsi_ble_on_remote_features_event(rsi_ble_event_remote_features_t *rsi_ble_event_remote_features)
{
  memcpy(&remote_dev_feature, rsi_ble_event_remote_features, sizeof(rsi_ble_event_remote_features_t));
  rsi_ble_app_set_event(RSI_BLE_RECEIVE_REMOTE_FEATURES);
}

/*============================================================================*/
/**
 * @fn         rsi_ble_data_length_change_event
 * @brief      invoked when data length is set
 * @param[out] rsi_ble_data_length_update, data length information
 * @section description
 * This Callback function indicates data length is set
 */
void rsi_ble_data_length_change_event(rsi_ble_event_data_length_update_t *rsi_ble_data_length_update)
{
  UNUSED_PARAMETER(
    rsi_ble_data_length_update); //This statement is added only to resolve compilation warning, value is unchanged
  // memcpy(&updated_data_len_params, rsi_ble_data_length_update, sizeof(rsi_ble_event_data_length_update_t));
  rsi_ble_app_set_event(RSI_BLE_DATA_LENGTH_CHANGE);
}
/*==============================================*/
/**
 * @fn         rsi_ble_on_mtu_event
 * @brief      invoked  when an MTU size event is received
 * @param[out]  rsi_ble_mtu, it indicates MTU size.
 * @return     none.
 * @section description
 * This callback function is invoked  when an MTU size event is received
 */
static void rsi_ble_on_mtu_event(rsi_ble_event_mtu_t *rsi_ble_mtu)
{
  memcpy(&app_ble_mtu_event, rsi_ble_mtu, sizeof(rsi_ble_event_mtu_t));
  rsi_6byte_dev_address_to_ascii(remote_dev_addr, app_ble_mtu_event.dev_addr);
  rsi_ble_app_set_event(RSI_BLE_MTU_EVENT);
}

/*==============================================*/
/**
 * @fn         rsi_ble_on_gatt_write_event
 * @brief      this is call back function, it invokes when write/notify events received.
 * @param[out]  event_id, it indicates write/notification event id.
 * @param[out]  rsi_ble_write, write event parameters.
 * @return     none.
 * @section description   手机APP-> BLE设备
 * This is a callback function
 */
static void rsi_ble_on_gatt_write_event(uint16_t event_id, rsi_ble_event_write_t *rsi_ble_write)
{
  UNUSED_PARAMETER(event_id);
  uint8_t cmdid;

  //  Requests will come from Mobile app
  if ((rsi_ble_mobile_to_device_hndl) == *((uint16_t *)rsi_ble_write->handle)) 
  {
    cmdid = rsi_ble_write->att_value[0];

    switch (cmdid) {
      case '3':{ // Scan command request
        LOG_PRINT("Received scan request\n");
        app_log_info("Received scan request\r\n");
        retry = 0;
        // memset(data, 0, sizeof(data));
        wifi_app_set_event(WIFI_APP_SCAN_STATE);
      } break;

      case '2':{ // Sending SSID
        memset(coex_ssid, 0, sizeof(coex_ssid));
        strcpy((char *)coex_ssid, (const char *)&rsi_ble_write->att_value[3]);

        rsi_ble_app_set_event(RSI_SSID);
      } break;

      case '5':{ // Sending Security type
        sec_type = ((rsi_ble_write->att_value[3]) - '0');
        LOG_PRINT("In Security Request\r\n");

        rsi_ble_app_set_event(RSI_SECTYPE);
      } break;

      case '6':{ // Sending PSK
        // memset(data, 0, sizeof(data));
        strcpy((char *)pwd, (const char *)&rsi_ble_write->att_value[3]);
        LOG_PRINT("PWD from ble app\r\n");
        wifi_app_set_event(WIFI_APP_JOIN_STATE);
      } break;

      case '7':{ // WLAN Status Request
        LOG_PRINT("WLAN status request received\r\n");
        // memset(data, 0, sizeof(data));
        if (connected) {
          rsi_ble_app_set_event(RSI_WLAN_ALREADY);
        } else {
          rsi_ble_app_set_event(RSI_WLAN_NOT_ALREADY);
        }
      } break;

      case '4':{ // WLAN disconnect request
        LOG_PRINT("WLAN disconnect request received\r\n");
        // memset(data, 0, sizeof(data));
        wifi_app_set_event(WIFI_APP_DISCONN_NOTIFY_STATE);
      } break;

      case '8':{ // FW version request
        // memset(data, 0, sizeof(data));
        rsi_ble_app_set_event(RSI_APP_FW_VERSION);
        LOG_PRINT("FW version request\r\n");
      } break;

      default:
        LOG_PRINT("Default command case \r\n\n");
        break;
    }
  }

  //  Requests will come from Mobile app
  CommandBufferT command_buffer_param;
  if ((cat_tx_handle) == *((uint16_t *)rsi_ble_write->handle)) 
  {
    app_log_info("<<----RECV----\n");
    app_log_hexdump_info(rsi_ble_write->att_value, rsi_ble_write->length);

    memcpy(command_buffer_param.data, rsi_ble_write->att_value, rsi_ble_write->length);
    command_buffer_param.len = rsi_ble_write->length;
    if(xQueueSend(bluetooth_data_queue, &command_buffer_param, 10 / portTICK_PERIOD_MS) == pdFALSE){
        app_log_error("Failed to send data because the queue was full!\r\n");
    }
  }
}

/*==============================================*/
/**
 * @fn         rsi_ble_app_init
 * @brief      initialize the BLE module.
 * @param[in]  none
 * @return     none.
 * @section description
 * This function is used to initialize the BLE module
 */
void rsi_ble_configurator_init(void)
{
  uint8_t adv[31] = { 2, 1, 6 };

  //  initializing the application events map
  ble_app_event_map = 0;

  rsi_ble_add_configurator_serv(); // adding simple BLE chat service
  rsi_ble_add_catcollar_serv();

  // registering the GAP callback functions
  rsi_ble_gap_register_callbacks(NULL,
                                 rsi_ble_on_connect_event,
                                 rsi_ble_on_disconnect_event,
                                 NULL,
                                 NULL,
                                 rsi_ble_data_length_change_event,
                                 rsi_ble_on_enhance_conn_status_event,
                                 NULL,
                                 rsi_ble_on_conn_update_complete_event,
                                 NULL);
  //! registering the GAP extended call back functions
  rsi_ble_gap_extended_register_callbacks(rsi_ble_on_remote_features_event, NULL);

  // registering the GATT callback functions
  rsi_ble_gatt_register_callbacks(NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  rsi_ble_on_gatt_write_event,
                                  NULL,
                                  NULL,
                                  NULL,
                                  rsi_ble_on_mtu_event,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

#ifdef RSI_BLE_PROVISIONING_ENABLED
  // Set local name
  rsi_bt_set_local_name((uint8_t *)RSI_BLE_APP_DEVICE_NAME);

  // prepare advertise data //local/device name
  adv[3] = strlen(RSI_BLE_APP_DEVICE_NAME) + 1;
  adv[4] = 9;
  strcpy((char *)&adv[5], RSI_BLE_APP_DEVICE_NAME);

  // set advertise data
  rsi_ble_set_advertise_data(adv, strlen(RSI_BLE_APP_DEVICE_NAME) + 5);
#else
  uint8_t catclooar_ble_name[16] = {0};
  uint8_t device_bt_addr[6] = {0};

  // Set local name
  const char *addr_str = RSI_BLE_ADV_DIR_ADDR;
  char addr_str_copy[20];
  strncpy(addr_str_copy, addr_str, sizeof(addr_str_copy));
  addr_str_copy[sizeof(addr_str_copy)-1] = '\0';

  char *token = strtok(addr_str_copy, ":");
  int i = 0;
  while (token != NULL && i < 6) {
      device_bt_addr[i++] = (uint8_t)strtol(token, NULL, 16);
      token = strtok(NULL, ":");
  }

  if (i != 6) {
      app_log_error("Failed to parse device bt addr, using default\r\n");
      // 使用默认地址
      uint8_t default_addr[6] = {0x00, 0x23, 0xA7, 0x12, 0x34, 0x56};
      memcpy(device_bt_addr, default_addr, sizeof(default_addr));
  }

  app_log_info("device bt addr hex = %02X:%02X:%02X:%02X:%02X:%02X\r\n",
               device_bt_addr[0], device_bt_addr[1], device_bt_addr[2],
               device_bt_addr[3], device_bt_addr[4], device_bt_addr[5]);

  snprintf((char *)catclooar_ble_name, sizeof(catclooar_ble_name), 
            "NDC%02X%02X%02X%02X%02X%02X", 
            device_bt_addr[0], device_bt_addr[1], device_bt_addr[2], 
            device_bt_addr[3], device_bt_addr[4], device_bt_addr[5]);
  app_log_info("ble local name = %s\r\n", catclooar_ble_name);
  rsi_bt_set_local_name((uint8_t *)catclooar_ble_name);
  rsi_bt_set_bd_addr((uint8_t *)device_bt_addr);

  // prepare advertise data //local/device name
  adv[3] = sizeof(catclooar_ble_name) + 1;
  adv[4] = 9;
  // strcpy((char *)&adv[5], catclooar_ble_name);
  memcpy(&adv[5], catclooar_ble_name, sizeof(catclooar_ble_name));

  // set advertise data
  rsi_ble_set_advertise_data(adv, sizeof(catclooar_ble_name) + 5);
#endif

  // set device in advertising mode.
  rsi_ble_start_advertising();
  LOG_PRINT("\r\nBLE Advertising Started...\r\n");
}

/*==============================================*/
/**
 * @fn         rsi_ble_app_task
 * @brief      this function will execute when BLE events are raised.
 * @param[in]  none.
 * @return     none.
 * @section description
 */

void rsi_ble_configurator_task(void *argument)
{
  UNUSED_PARAMETER(argument);

  int32_t status = 0;
  int32_t event_id;
  uint8_t data[RSI_BLE_PROV_MAX_DATA_LEN] = { 0 };
  uint8_t scan_ix, length;
  uint8_t k;

  scanresult = (sl_wifi_scan_result_t *)malloc(scanbuf_size);
  if (scanresult == NULL) {
    LOG_PRINT("Failed to allocate memory for scan result\n");
    return;
  }
  memset(scanresult, 0, scanbuf_size);
  while (1) {
    // checking for events list
    event_id = rsi_ble_app_get_event();

    if (event_id == -1) {
      osSemaphoreAcquire(ble_thread_sem, osWaitForever);
      // if events are not received loop will be continued.
      continue;
    }

    switch (event_id) {
      case RSI_BLE_ENH_CONN_EVENT: {
        // event invokes when connection was completed

        // clear the served event
        rsi_ble_app_clear_event(RSI_BLE_ENH_CONN_EVENT);

        //MTU exchange
        status = rsi_ble_mtu_exchange_event(conn_event_to_app.dev_addr, BLE_MTU_SIZE);
        if (status != RSI_SUCCESS) {
          LOG_PRINT("\n MTU request failed with error code %lx", status);
        }
        status = rsi_ble_conn_params_update(conn_event_to_app.dev_addr,
                                            CONN_INTERVAL_DEFAULT_MIN,
                                            CONN_INTERVAL_DEFAULT_MAX,
                                            CONNECTION_LATENCY,
                                            SUPERVISION_TIMEOUT);
        if (status != RSI_SUCCESS) {
          LOG_PRINT("\n rsi_ble_conn_params_update command failed : %lx", status);
        }

        catcollar_bt_connection_set_state(CATCOLLAR_BT_CONNECTED);

        app_log_info("Connected to the device: %s\r\n",
                     rsi_6byte_dev_address_to_ascii(remote_dev_addr, conn_event_to_app.dev_addr));
      } break;

      case RSI_BLE_DISCONN_EVENT: {
        // event invokes when disconnection was completed

        // clear the served event
        rsi_ble_app_clear_event(RSI_BLE_DISCONN_EVENT);
        LOG_PRINT("\r\nDisconnected - remote_dev_addr : %s\r\n",
                  rsi_6byte_dev_address_to_ascii(remote_dev_addr, disconn_event_to_app.dev_addr));

        // set device in advertising mode.
adv:
        status = rsi_ble_start_advertising();
        if (status != RSI_SUCCESS) {
          goto adv;
        } else {
          LOG_PRINT("\r\nStarted Advertising \n");
        }

        catcollar_bt_connection_set_state(CATCOLLAR_BT_DISCONNECTED);

      } break;

      case RSI_APP_FW_VERSION: {
        sl_wifi_firmware_version_t firmware_version = { 0 };

        rsi_ble_app_clear_event(RSI_APP_FW_VERSION);
        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);

        status = sl_wifi_get_firmware_version(&firmware_version);
        if (status == SL_STATUS_OK) {
          data[0] = 0x08;
          data[1] = sizeof(sl_wifi_firmware_version_t);
          memcpy(&data[2], &firmware_version, sizeof(sl_wifi_firmware_version_t));

          rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
          print_firmware_version(&firmware_version);
        }
      } break;

        // Connected SSID name (response to '7' command if connection is already established)
      case RSI_WLAN_ALREADY: {
        rsi_ble_app_clear_event(RSI_WLAN_ALREADY);

        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);

        data[1] = connected; /*This index will indicate wlan AP connect or disconnect status to Android app*/
        data[0] = 0x07;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
      } break;

        // NO WLAN connection (response to '7' command if connection is there already)
      case RSI_WLAN_NOT_ALREADY: {
        rsi_ble_app_clear_event(RSI_WLAN_NOT_ALREADY);
        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[0] = 0x07;
        data[1] = 0x00;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
      } break;

      case RSI_BLE_WLAN_DISCONN_NOTIFY: {
        rsi_ble_app_clear_event(RSI_BLE_WLAN_DISCONN_NOTIFY);
        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[1] = 0x01;
        data[0] = 0x04;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
      } break;

      case RSI_BLE_WLAN_TIMEOUT_NOTIFY: {
        rsi_ble_app_clear_event(RSI_BLE_WLAN_TIMEOUT_NOTIFY);
        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[0] = 0x02;
        data[1] = 0x00;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
      } break;

      case RSI_BLE_WLAN_DISCONNECT_STATUS: {
        rsi_ble_app_clear_event(RSI_BLE_WLAN_DISCONNECT_STATUS);
        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[0] = 0x01;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
      } break;

      case RSI_SSID: {
        rsi_ble_app_clear_event(RSI_SSID);
      } break;

      case RSI_SECTYPE: {
        rsi_ble_app_clear_event(RSI_SECTYPE);
        if (sec_type == 0) {
          wifi_app_set_event(WIFI_APP_JOIN_STATE);
        }
      } break;

        // Scan results from device (response to '3' command)
      case RSI_BLE_WLAN_SCAN_RESP: //Send the SSID data to mobile ble application WYZBEE CONFIGURATOR
      {
        rsi_ble_app_clear_event(RSI_BLE_WLAN_SCAN_RESP); // clear the served event

        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[0] = 0x03;
        data[1] = scanresult->scan_count;
        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);

        for (scan_ix = 0; scan_ix < scanresult->scan_count; scan_ix++) {
          memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
          data[0] = scanresult->scan_info[scan_ix].security_mode;
          data[1] = ',';
          strcpy((char *)data + 2, (const char *)scanresult->scan_info[scan_ix].ssid);
          length = strlen((char *)data + 2);
          length = length + 2;

          rsi_ble_set_local_att_value(rsi_ble_att3_val_hndl, RSI_BLE_PROV_MAX_DATA_LEN, data);
          osDelay(10);
        }

        LOG_PRINT("Displayed scan list in Silabs app\r\n\n");
      } break;

      // WLAN connection response status (response to '2' command)
      case RSI_BLE_WLAN_JOIN_STATUS: //Send the connected status to mobile ble application WYZBEE CONFIGURATOR
      {
        sl_mac_address_t mac_addr = { 0 };

        sl_ip_address_t ip = { 0 };
        ip.type            = ip_address.type;
        ip.ip.v4.value     = ip_address.ip.v4.ip_address.value;

        // clear the served event
        rsi_ble_app_clear_event(RSI_BLE_WLAN_JOIN_STATUS);

        memset(data, 0, RSI_BLE_PROV_MAX_DATA_LEN);
        data[0] = 0x02;
        data[1] = 0x01;
        data[2] = ',';

        // Copy the MAC address
        status = sl_wifi_get_mac_address(SL_WIFI_CLIENT_INTERFACE, &mac_addr);
        if (status == SL_STATUS_OK) {
          for (k = 0; k < 6; k++) {
            data[k + 3] = mac_addr.octet[k];
          }
        } else {
          k = 6;
        }
        data[k + 3] = ',';

        // IP Address
        for (int i = 0; k < 10; k++, i++) {
          data[k + 4] = ip.ip.v4.bytes[i];
        }

        rsi_ble_set_local_att_value(rsi_ble_device_to_mobile_hndl,
                                    RSI_BLE_PROV_MAX_DATA_LEN,
                                    data); // set the local attribute value.
        LOG_PRINT("AP joined successfully\n\n");
      } break;
      case RSI_BLE_MTU_EVENT: {
        //! clear the served event
        rsi_ble_app_clear_event(RSI_BLE_MTU_EVENT);
        //! event invokes when write/notification events received
      } break;
      case RSI_BLE_CONN_UPDATE_EVENT: {
        rsi_ble_app_clear_event(RSI_BLE_CONN_UPDATE_EVENT);

      } break;
      case RSI_BLE_RECEIVE_REMOTE_FEATURES: {
        //! clear the served event
        rsi_ble_app_clear_event(RSI_BLE_RECEIVE_REMOTE_FEATURES);

        if (remote_dev_feature.remote_features[0] & 0x20) {
          status = rsi_ble_set_data_len(conn_event_to_app.dev_addr, TX_LEN, TX_TIME);
          if (status != RSI_SUCCESS) {
            LOG_PRINT("\n set data length cmd failed with error code = "
                      "%lx \n",
                      status);
            rsi_ble_app_set_event(RSI_BLE_RECEIVE_REMOTE_FEATURES);
          }
        }

      } break;
      case RSI_BLE_DATA_LENGTH_CHANGE: {
        //! clear the served event
        rsi_ble_app_clear_event(RSI_BLE_DATA_LENGTH_CHANGE);
      } break;
      default:
        break;
    }
  }

  free(scanresult);
}

/*==============================================*/
/**
 * @fn         wifi_app_send_to_ble
 * @brief      this function is used to send data to ble app.
 * @param[in]   msg_type, it indicates write/notification event id.
 * @param[in]  data, raw data pointer.
 * @param[in]  data_len, raw data length.
 * @return     none.
 * @section description
 */
void wifi_app_send_to_ble(uint16_t msg_type, uint8_t *data, uint16_t data_len)
{
  switch (msg_type) {
    case WIFI_APP_SCAN_RESP:
      memset(scanresult, 0, data_len);
      memcpy(scanresult, (sl_wifi_scan_result_t *)data, data_len);

      rsi_ble_app_set_event(RSI_BLE_WLAN_SCAN_RESP);
      break;
    case WIFI_APP_CONNECTION_STATUS:
      rsi_ble_app_set_event(RSI_BLE_WLAN_JOIN_STATUS);
      break;
    case WIFI_APP_DISCONNECTION_STATUS:
      rsi_ble_app_set_event(RSI_BLE_WLAN_DISCONNECT_STATUS);
      break;
    case WIFI_APP_DISCONNECTION_NOTIFY:
      rsi_ble_app_set_event(RSI_BLE_WLAN_DISCONN_NOTIFY);
      break;
    case WIFI_APP_TIMEOUT_NOTIFY:
      rsi_ble_app_set_event(RSI_BLE_WLAN_TIMEOUT_NOTIFY);
      break;
    default:
      break;
  }
}
