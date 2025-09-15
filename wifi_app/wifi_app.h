#ifndef WIFI_APP_H
#define WIFI_APP_H

#include "sl_status.h"

typedef enum {
  CATCOLLAR_WIFI_DISCONNECTED = 0,
  CATCOLLAR_WIFI_CONNECTED
} catcollar_wifi_connection_state_t;

typedef struct
{
    char ssid[32];
    char pwd[64];
}bsp_wifi_info_t;


sl_status_t wifi_client_connect(char *ssid, char *password, sl_wifi_security_t security_type);
int catcollar_wifi_connection_get_state(void);
void catcollar_wifi_connection_set_state(catcollar_wifi_connection_state_t state);

/// test
void wifi_connect_test(void);

#endif // WIFI_APP_H