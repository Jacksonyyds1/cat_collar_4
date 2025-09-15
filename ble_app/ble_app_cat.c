// //! SL Wi-Fi SDK includes
// #include "sl_board_configuration.h"
// #include "sl_constants.h"
// #include "sl_wifi.h"
// #include "sl_net_ip_types.h"
// #include "cmsis_os2.h"
// #include "sl_utility.h"

// BLE include file to refer BLE APIs
#include <rsi_ble_apis.h>
#include <rsi_bt_common_apis.h>
#include <rsi_common_apis.h>
#include <string.h>
// #include <rsi_ble_common_config.h>

#include "ble_config.h"
// #include "wifi_config.h"

#include "app_log.h"
#include "ble_app_cat.h"
#include "ble_data_parse.h"
#include "chekr_dash.h"
#include "blinky.h"
#include "common.h"


uint16_t cat_tx_handle, cat_rx_handle, cat_notify_handle;

uuid_t cat_service = {0};
uuid_t cat_rx_char_service = {0};
uuid_t cat_tx_char_service = {0};
uuid_t cat_notify_char_service = {0};

rsi_ble_resp_add_serv_t cat_serv_response;

sl_wifi_firmware_version_t version = {0};

/*============================================================================*/
/**
 * @fn         rsi_ble_add_char_serv_att
 * @brief      this function is used to add characteristic service attribute.
 * @param[in]  serv_handler, service handler.
 * @param[in]  handle, characteristic service attribute handle.
 * @param[in]  val_prop, characteristic value property.
 * @param[in]  att_val_handle, characteristic value handle
 * @param[in]  att_val_uuid, characteristic value UUID
 * @return     none.
 * @section description
 * This function is used at application to add characteristic attribute
 */
static void rsi_ble_add_char_serv_att(void *serv_handler, uint16_t handle,
                                      uint8_t val_prop, uint16_t att_val_handle,
                                      uuid_t att_val_uuid) {
  rsi_ble_req_add_att_t new_att = {0};

  //! preparing the attribute service structure
  new_att.serv_handler = serv_handler;
  new_att.handle = handle;
  new_att.att_uuid.size = 2;
  new_att.att_uuid.val.val16 = RSI_BLE_CHAR_SERV_UUID;
  new_att.property = RSI_BLE_ATT_PROPERTY_READ;

  //! preparing the characteristic attribute value
  if (att_val_uuid.size == UUID_SIZE) {
    new_att.data_len = 4 + att_val_uuid.size;
    new_att.data[0] = val_prop;
    rsi_uint16_to_2bytes(&new_att.data[2], att_val_handle);
    memcpy(&new_att.data[4], &att_val_uuid.val.val128, sizeof(att_val_uuid.val.val128));
  } else {
    new_att.data_len = 4;
    new_att.data[0] = val_prop;
    rsi_uint16_to_2bytes(&new_att.data[2], att_val_handle);
    rsi_uint16_to_2bytes(&new_att.data[4], att_val_uuid.val.val16);
  }

  //! Add attribute to the service
  rsi_ble_add_attribute(&new_att);

  return;
}
/*============================================================================*/
/**
 * @fn         rsi_ble_add_char_val_att
 * @brief      this function is used to add characteristic value attribute.
 * @param[in]  serv_handler, new service handler.
 * @param[in]  handle, characteristic value attribute handle.
 * @param[in]  att_type_uuid, attribute UUID value.
 * @param[in]  val_prop, characteristic value property.
 * @param[in]  data, characteristic value data pointer.
 * @param[in]  data_len, characteristic value length.
 * @return     none.
 * @section description
 * This function is used to add characteristic value attribute.
 */

static void rsi_ble_add_char_val_att(void *serv_handler, 
                                     uint16_t handle,
                                     uuid_t att_type_uuid, 
                                     uint8_t val_prop,
                                     uint8_t *data, 
                                     uint8_t data_len,
                                     uint8_t auth_read)
{
  rsi_ble_req_add_att_t new_att = {0};

  //! preparing the attributes
  new_att.serv_handler = serv_handler;
  new_att.handle       = handle;
  new_att.config_bitmap = auth_read;
  memcpy(&new_att.att_uuid, &att_type_uuid, sizeof(uuid_t));
  new_att.property = val_prop;

  // preparing the attribute value
  new_att.data_len = RSI_MIN(sizeof(new_att.data), data_len);
  memcpy(new_att.data, data, new_att.data_len);

  // add attribute to the service
  rsi_ble_add_attribute(&new_att);

  // check the attribute property with notification
  if (val_prop & RSI_BLE_ATT_PROPERTY_NOTIFY) {
    // if notification property supports then we need to add client characteristic service.

    // preparing the client characteristic attribute & values
    memset(&new_att, 0, sizeof(rsi_ble_req_add_att_t));
    new_att.serv_handler       = serv_handler;
    new_att.handle             = handle + 1;
    new_att.att_uuid.size      = 2;
    new_att.att_uuid.val.val16 = RSI_BLE_CLIENT_CHAR_UUID;
    new_att.property           = RSI_BLE_ATT_PROPERTY_READ | RSI_BLE_ATT_PROPERTY_WRITE;
    new_att.data_len           = 2;

    // add attribute to the service
    rsi_ble_add_attribute(&new_att);
  }

  return;
}
/*============================================================================*/
/**
 * @fn         rsi_ble_prepare_128bit_uuid
 * @brief      this function is used to prepare the 128bit UUID
 * @param[in]  temp_service,received 128-bit service.
 * @param[out] temp_uuid,formed 128-bit service structure.
 * @return     none.
 * @section description
 * This function prepares the 128bit UUID
 */
static void rsi_ble_prepare_128bit_uuid(uint8_t temp_service[UUID_SIZE],
                                        uuid_t *temp_uuid) {
  temp_uuid->val.val128.data1 =
      ((temp_service[0] << 24) | (temp_service[1] << 16) |
       (temp_service[2] << 8) | (temp_service[3]));
  temp_uuid->val.val128.data2 = ((temp_service[5]) | (temp_service[4] << 8));
  temp_uuid->val.val128.data3 = ((temp_service[7]) | (temp_service[6] << 8));
  temp_uuid->val.val128.data4[0] = temp_service[9];
  temp_uuid->val.val128.data4[1] = temp_service[8];
  temp_uuid->val.val128.data4[2] = temp_service[11];
  temp_uuid->val.val128.data4[3] = temp_service[10];
  temp_uuid->val.val128.data4[4] = temp_service[15];
  temp_uuid->val.val128.data4[5] = temp_service[14];
  temp_uuid->val.val128.data4[6] = temp_service[13];
  temp_uuid->val.val128.data4[7] = temp_service[12];
}

/*============================================================================*/
/**
 * @fn         rsi_ble_add_cat_service
 * @brief      this function is used to create the OTA Service
 * @param[in]  none.
 * @return     none.
 * @section description
 * This function is used to create the OTA service
 * UUID: 00c74f02-d1bc-11ed-afa1-0242ac120002
 */

static void rsi_ble_add_cat_service(void) {

  rsi_ble_resp_add_serv_t new_serv_resp = {0};
  uint8_t cat_service_uuid[UUID_SIZE] = {0x00, 0xc7, 0x4f, 0x02, 0xd1, 0xbc,
                                  0x11, 0xed, 0xaf, 0xa1, 0x02, 0x42,
                                  0xac, 0x12, 0x00, 0x02};
  rsi_ble_prepare_128bit_uuid(cat_service_uuid, &cat_service);
  cat_service.size = UUID_SIZE;
  rsi_ble_add_service(cat_service, &new_serv_resp);
  cat_serv_response.serv_handler = new_serv_resp.serv_handler;
  cat_serv_response.start_handle = new_serv_resp.start_handle;
  app_log_info("cat_serv_response.start_handle = %d\r\n", cat_serv_response.start_handle);
}

/*============================================================================*/
/**
 * @fn         rsi_ble_add_cat_tx_char
 * @brief      this function is used to create the "Cat TX" charactersitic service
 * UUID: 6765a69d-cd79-4df6-aad5-043df9425556
 */
static void rsi_ble_add_cat_tx_char(void) {
  uint8_t send_data[RSI_BLE_CAT_MAX_DATA_LEN] = {0};
  uint8_t cat_tx_char_serv[UUID_SIZE] = {0x67, 0x65, 0xa6, 0x9d, 0xcd, 0x79,
                                    0x4d, 0xf6, 0xaa, 0xd5, 0x04, 0x3d,
                                    0xf9, 0x42, 0x55, 0x56};

  cat_tx_char_service.size = UUID_SIZE;
  rsi_ble_prepare_128bit_uuid(cat_tx_char_serv, &cat_tx_char_service);

  rsi_ble_add_char_serv_att(cat_serv_response.serv_handler,
                            cat_serv_response.start_handle + 1,
                             RSI_BLE_ATT_PROPERTY_WRITE,
                            cat_serv_response.start_handle + 2,
                            cat_tx_char_service);

  rsi_ble_add_char_val_att(cat_serv_response.serv_handler,
                           cat_serv_response.start_handle + 2,
                           cat_tx_char_service,
                           RSI_BLE_ATT_PROPERTY_WRITE,
                           send_data,
                           sizeof(send_data),
                           0);
  cat_tx_handle = cat_serv_response.start_handle + 2;
  app_log_info("cat_tx_handle = %d\r\n", cat_tx_handle);
}

/*============================================================================*/
/**
 * @fn         rsi_ble_add_cat_rx_char
 * @brief      this function is used to create the "Cat RX" characteristic service
 * UUID: b6ab2ce3-a5aa-436a-817a-cc13a45aab76
 */
static void rsi_ble_add_cat_rx_char(void) {
  uint8_t read_data[RSI_BLE_CAT_MAX_DATA_LEN] = {0};
  uint8_t cat_rx_char_serv[UUID_SIZE] = {0xb6, 0xab, 0x2c, 0xe3, 0xa5, 0xaa,
                                    0x43, 0x6a, 0x81, 0x7a, 0xcc, 0x13,
                                    0xa4, 0x5a, 0xab, 0x76};
  cat_rx_char_service.size = UUID_SIZE;
  rsi_ble_prepare_128bit_uuid(cat_rx_char_serv, &cat_rx_char_service);

  rsi_ble_add_char_serv_att(cat_serv_response.serv_handler,
                            cat_serv_response.start_handle + 3,
                            RSI_BLE_ATT_PROPERTY_READ,
                            cat_serv_response.start_handle + 4,
                            cat_rx_char_service);

  rsi_ble_add_char_val_att(cat_serv_response.serv_handler,
                            cat_serv_response.start_handle + 4,
                            cat_rx_char_service,
                            RSI_BLE_ATT_PROPERTY_READ,
                            read_data,
                            sizeof(read_data),
                            BIT(7));
  cat_rx_handle = cat_serv_response.start_handle + 4;
  app_log_info("cat_rx_handle = %d\r\n", cat_rx_handle);
}

/*============================================================================*/
/**
 * @fn         rsi_ble_add_cat_notify_char
 * @brief      This function is used to create "Cat Notify" characteristic service
 * UUID:  207bdc30-c3cc-4a14-8b66-56ba8a826640
 */
static void rsi_ble_add_cat_notify_char(void) {
  uint8_t notify_data[RSI_BLE_CAT_MAX_DATA_LEN] = {0};
  uint8_t cat_notify_char_serv[UUID_SIZE] = {0x20, 0x7b, 0xdc, 0x30, 0xc3, 0xcc,
                                      0x4a, 0x14, 0x8b, 0x66, 0x56, 0xba,
                                      0x8a, 0x82, 0x66, 0x40};
  cat_notify_char_service.size = UUID_SIZE;
  rsi_ble_prepare_128bit_uuid(cat_notify_char_serv, &cat_notify_char_service);

  rsi_ble_add_char_serv_att(cat_serv_response.serv_handler,
                            cat_serv_response.start_handle + 5,
                            RSI_BLE_ATT_PROPERTY_NOTIFY,
                            cat_serv_response.start_handle + 6,
                            cat_notify_char_service);

  rsi_ble_add_char_val_att(cat_serv_response.serv_handler,
                           cat_serv_response.start_handle + 6,
                           cat_notify_char_service,
                           RSI_BLE_ATT_PROPERTY_NOTIFY,
                           notify_data,
                           sizeof(notify_data),
                           0);
  cat_notify_handle = cat_serv_response.start_handle + 6;
  app_log_info("cat_notify_handle = %d\r\n", cat_notify_handle);
}

void rsi_ble_add_catcollar_serv(void)
{
  //! Adding the "CAT Service" Primary service
  //! UUID: 00c74f02-d1bc-11ed-afa1-0242ac120002
  rsi_ble_add_cat_service();

  //! Adding the "Cat TX" characteristic service attribute
  //! UUID: 6765a69d-cd79-4df6-aad5-043df9425556
  // mobile app will use this handle to send data to the device
  // mobile -> device
  rsi_ble_add_cat_tx_char();

  //! Adding the "Cat RX" characteristic service attribute
  //! UUID: b6ab2ce3-a5aa-436a-817a-cc13a45aab76
  // device will use this handle to send data to the mobile app
  // device -> mobile
  rsi_ble_add_cat_rx_char();

  //! Adding the "Cat Notify" characteristic service attribute
  //! UUID: 207bdc30-c3cc-4a14-8b66-56ba8a826640
  // mobile app will use this handle to receive notifications from the device
  // mobile -> device
  rsi_ble_add_cat_notify_char();

}


static catcollar_bt_connection_state_t catcollar_bt_connection_state = 0;
int catcollar_bt_connection_get_state(void)
{
  return catcollar_bt_connection_state;
}

void catcollar_bt_connection_set_state(catcollar_bt_connection_state_t state)
{
  catcollar_bt_connection_state = state;
  if (state == CATCOLLAR_BT_DISCONNECTED) {
    leds_play(BLUE_LED, LEDS_SLOW_BLINK);
  } else if (state == CATCOLLAR_BT_CONNECTED) {
    leds_play(BLUE_LED, LEDS_ON);
  } else {
    app_log_error("Unknown connection state: %d\r\n", state);
    leds_play(BLUE_LED, LEDS_OFF);
  }
}

static char local_name_str[16];
rsi_bt_resp_get_local_name_t local_bt_name;
char *ble_get_local_name(void)
{
  rsi_bt_get_local_name(&local_bt_name);
  if (local_bt_name.name_len > 0) {
    memcpy(local_name_str, local_bt_name.name, local_bt_name.name_len);
    local_name_str[local_bt_name.name_len] = '\0'; // Null-terminate the string
    return local_name_str;
  } else {
    return "Unknown";
  }
}

static read_response_t read_response;
// send notification
void write_to_central(uint8_t *data, int len)
{
	if (catcollar_bt_connection_get_state() == CATCOLLAR_BT_CONNECTED) {
    app_log_info("----SEND---->>\n");
    // app_log_info("HEX Data:\n");
    app_log_hexdump_info(data, len);
    memcpy(read_response.response, data, len);
    read_response.response_len = len;
    rsi_ble_set_local_att_value(cat_rx_handle, read_response.response_len, read_response.response);
	} else {
		app_log_error("BLE disconnected\r\n");
	}
}

int chekr_service_init(void)
{
	// create ChekrAppLink service
	dashboard_init();

  // ///test dashboard
  // dashboard_ctrl(DASH_START, 3);
	return 0;
}

