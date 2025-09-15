/***************************************************************************/ /**
 * @file app.h
 * @brief Top level application functions
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef APP_H
#define APP_H

#include "sl_board_configuration.h"
#include "sl_wifi.h"
#include "ble_config.h"

//A40DD31369C5
// sl_mac_address_t cat_wifi_mac = { .octet = { 0xA4, 0x0D, 0xD3, 0x13, 0x69, 0xC5 } };

static const sl_wifi_device_configuration_t cat_wifi_client_config = 
{
             .boot_option = LOAD_NWP_FW,
             .mac_address = NULL,
             .band        = SL_SI91X_WIFI_BAND_2_4GHZ,
             .region_code = US,
             .boot_config = 
             {
               .oper_mode       = SL_SI91X_CLIENT_MODE,
               .coex_mode       = SL_SI91X_WLAN_BLE_MODE,
               .feature_bit_map = (SL_SI91X_FEAT_ULP_GPIO_BASED_HANDSHAKE | SL_SI91X_FEAT_DEV_TO_HOST_ULP_GPIO_1
#ifdef SLI_SI91X_MCU_INTERFACE
                                   | SL_SI91X_FEAT_WPS_DISABLE
#endif
                                   ),
               .tcp_ip_feature_bit_map = (SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID),
               .custom_feature_bit_map = (SL_SI91X_CUSTOM_FEAT_EXTENTION_VALID | SL_SI91X_CUSTOM_FEAT_EXTENTION_VALID),
               .ext_custom_feature_bit_map =
                 (SL_SI91X_EXT_FEAT_LOW_POWER_MODE | SL_SI91X_EXT_FEAT_XTAL_CLK | MEMORY_CONFIG
#if defined(SLI_SI917) || defined(SLI_SI915)
                  | SL_SI91X_EXT_FEAT_FRONT_END_INTERNAL_SWITCH
                  // | SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0
#endif // SLI_SI917
                  | SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE),
               .bt_feature_bit_map         = (SL_SI91X_BT_RF_TYPE | SL_SI91X_ENABLE_BLE_PROTOCOL),
               .ext_tcp_ip_feature_bit_map = (SL_SI91X_CONFIG_FEAT_EXTENTION_VALID),
               //!ENABLE_BLE_PROTOCOL in bt_feature_bit_map
               .ble_feature_bit_map =
                 ((SL_SI91X_BLE_MAX_NBR_PERIPHERALS(RSI_BLE_MAX_NBR_PERIPHERALS)
                   | SL_SI91X_BLE_MAX_NBR_CENTRALS(RSI_BLE_MAX_NBR_CENTRALS)
                   | SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV)
                   | SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC))
                  | SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENTION_VALID | SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX)
                  | SL_SI91X_BLE_PWR_SAVE_OPTIONS(RSI_BLE_PWR_SAVE_OPTIONS) | SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE
#if RSI_BLE_GATT_ASYNC_ENABLE
                  | SL_SI91X_BLE_GATT_ASYNC_ENABLE
#endif
                  ),

               .ble_ext_feature_bit_map =
                 ((SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS)
                   | SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES))
#if RSI_BLE_INDICATE_CONFIRMATION_FROM_HOST
                  | SL_SI91X_BLE_INDICATE_CONFIRMATION_FROM_HOST //indication response from app
#endif
#if RSI_BLE_MTU_EXCHANGE_FROM_HOST
                  | SL_SI91X_BLE_MTU_EXCHANGE_FROM_HOST //MTU Exchange request initiation from app
#endif
#if RSI_BLE_SET_SCAN_RESP_DATA_FROM_HOST
                  | (SL_SI91X_BLE_SET_SCAN_RESP_DATA_FROM_HOST) //Set SCAN Resp Data from app
#endif
#if RSI_BLE_DISABLE_CODED_PHY_FROM_HOST
                  | (SL_SI91X_BLE_DISABLE_CODED_PHY_FROM_HOST) //Disable Coded PHY from app
#endif
#if BLE_SIMPLE_GATT
                  | SL_SI91X_BLE_GATT_INIT
#endif
                  ),
               .config_feature_bit_map = (SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP) 
              }
};

// const sl_wifi_device_configuration_t cat_wifi_client_config = {
//     .boot_option = LOAD_NWP_FW,
//     .mac_address = NULL,
//     .band        = SL_SI91X_WIFI_BAND_2_4GHZ,
//     .boot_config = { 
//                    .oper_mode                  = SL_SI91X_CLIENT_MODE,
//                    .coex_mode                  = SL_SI91X_WLAN_BLE_MODE,
//                    .feature_bit_map            = (SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_AGGREGATION),
//                    .tcp_ip_feature_bit_map     = (SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT | SL_SI91X_TCP_IP_FEAT_HTTP_CLIENT
//                                               | SL_SI91X_TCP_IP_FEAT_DNS_CLIENT | SL_SI91X_TCP_IP_FEAT_SSL | SL_SI91X_TCP_IP_FEAT_ICMP | SL_SI91X_TCP_IP_FEAT_SMTP_CLIENT),
//                    .custom_feature_bit_map     =                      (SL_SI91X_CUSTOM_FEAT_EXTENTION_VALID
//                        | SL_SI91X_CUSTOM_FEAT_ASYNC_CONNECTION_STATUS
//                        | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID),
//                    .ext_custom_feature_bit_map = (SL_SI91X_EXT_FEAT_SSL_VERSIONS_SUPPORT
//                        | SL_SI91X_EXT_FEAT_XTAL_CLK
//                        | MEMORY_CONFIG
//                        | SL_SI91X_EXT_FEAT_RSA_KEY_WITH_4096_SUPPORT
//                        | SL_SI91X_EXT_FEAT_SSL_CERT_WITH_4096_KEY_SUPPORT
// #if defined(SLI_SI917) || defined(SLI_SI915)
//                                                   | SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0
// #endif
//                                                   ),
//                    .bt_feature_bit_map      = 0,
//                    .ext_tcp_ip_feature_bit_map =
//                      (SL_SI91X_EXT_TCP_IP_WINDOW_SCALING
//                       | SL_SI91X_EXT_TCP_IP_TOTAL_SELECTS(10)
//                       | SL_SI91X_EXT_TCP_IP_FEAT_SSL_THREE_SOCKETS
//                       | SL_SI91X_EXT_TCP_IP_FEAT_SSL_MEMORY_CLOUD),
//                    .ble_feature_bit_map     = 0,
//                    .ble_ext_feature_bit_map = 0,
//                    .config_feature_bit_map  = 0 
//                   }
// };

// const sl_wifi_device_configuration_t station_init_configuration = {
//   .boot_option = LOAD_NWP_FW,
//   .mac_address = NULL,
//   .band        = SL_SI91X_WIFI_BAND_2_4GHZ,
//   .boot_config = { .oper_mode = SL_SI91X_CLIENT_MODE,
//                    .coex_mode = SL_SI91X_WLAN_ONLY_MODE,
//                    .feature_bit_map =
//                      (SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_AGGREGATION | SL_SI91X_FEAT_ULP_GPIO_BASED_HANDSHAKE
// #ifdef SLI_SI91X_MCU_INTERFACE
//                       | SL_SI91X_FEAT_WPS_DISABLE
// #endif
//                       ),
//                    .tcp_ip_feature_bit_map     = (SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT | SL_SI91X_TCP_IP_FEAT_DNS_CLIENT
//                                               | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID),
//                    .custom_feature_bit_map     = (SL_SI91X_CUSTOM_FEAT_EXTENTION_VALID),
//                    .ext_custom_feature_bit_map = (SL_SI91X_EXT_FEAT_LOW_POWER_MODE | SL_SI91X_EXT_FEAT_XTAL_CLK
//                                                   | SL_SI91X_EXT_FEAT_UART_SEL_FOR_DEBUG_PRINTS | MEMORY_CONFIG
// #if defined(SLI_SI917) || defined(SLI_SI915)
//                                                   | SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0
// #endif
//                                                   ),
//                    .bt_feature_bit_map         = 0,
//                    .ext_tcp_ip_feature_bit_map = SL_SI91X_CONFIG_FEAT_EXTENTION_VALID,
//                    .ble_feature_bit_map        = 0,
//                    .ble_ext_feature_bit_map    = 0,
//                    .config_feature_bit_map = (SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP | SL_SI91X_ENABLE_ENHANCED_MAX_PSP) }
// };

/***************************************************************************/ /**
 * Initialize application.
 ******************************************************************************/
void app_init(void);

/***************************************************************************/ /**
 * App ticking function.
 ******************************************************************************/
void app_process_action(void);

#endif // APP_H
