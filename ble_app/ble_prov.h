#ifndef __BLE_PROV_H__
#define __BLE_PROV_H__

#include <stdint.h>
// #include <stdbool.h>

// max data length
#define RSI_BLE_PROV_MAX_DATA_LEN   66
// local device name
#define RSI_BLE_APP_DEVICE_NAME "BLE_CONFIGURATOR"

extern uint8_t rsi_ble_mobile_to_device_hndl;
extern uint16_t rsi_ble_device_to_mobile_hndl;
extern uint16_t rsi_ble_att3_val_hndl;

extern uint32_t rsi_ble_add_configurator_serv(void);

#endif // __BLE_PROV_H__