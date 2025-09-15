#ifndef HTTP_EVENT_H
#define HTTP_EVENT_H

#include "sl_status.h"

/******************************************************
 *                      Macros
 ******************************************************/
#define HTTP_SYNC_RESPONSE  0
#define HTTP_ASYNC_RESPONSE 1
#define CLEAN_HTTP_CLIENT_IF_FAILED(status, client_handle, is_sync) \
  {                                                                 \
    if (status != SL_STATUS_OK) {                                   \
      sl_http_client_deinit(client_handle);                         \
      return ((is_sync == 0) ? status : callback_status);           \
    }                                                               \
  }

//! Set IP version
#define IP_VERSION   SL_IPV4
//! Set HTTP version
#define HTTP_VERSION SL_HTTP_V_1_1
//! Set 1 - Enable and 0 - Disable
#define HTTPS_ENABLE 1
//! Set 1 - Enable and 0 - Disable
#define EXTENDED_HEADER_ENABLE 0


#if HTTPS_ENABLE
//! Set TLS version
#define TLS_VERSION SL_TLS_V_1_2
#define CERTIFICATE_INDEX SL_HTTPS_CLIENT_CERTIFICATE_INDEX_1
// //! Load certificate to device flash :
// //! Certificate could be loaded once and need not be loaded for every boot up
// #define LOAD_CERTIFICATE 1
#endif
//! Server port number
#if HTTPS_ENABLE
//! Set default HTTPS port
#define HTTP_PORT 443
#else
//! Set default HTTP port
#define HTTP_PORT 80
#endif

#define DNS_TIMEOUT         20000
#define MAX_DNS_RETRY_COUNT 5
#define RSI_TCP_IP_BYPASS   RSI_DISABLE

//! Application buffer length
#define APP_BUFFER_LENGTH 2000

#if EXTENDED_HEADER_ENABLE
//! Extended headers
#define KEY1 "Content-Type"
#define VAL1 "text/html; charset=utf-8"

#define KEY2 "Content-Encoding"
#define VAL2 "br"

#define KEY3 "Content-Language"
#define VAL3 "de-DE"

#define KEY4 "Content-Location"
#define VAL4 "/index.html"
#endif

#define HTTPS_DOWNLOAD    1  //! Enable this to test HTTPS download and also set RX_DATA to '1'
#if HTTPS_DOWNLOAD
#define SSL              1   //! Enable SSL or not
#define LOAD_CERTIFICATE 1   //! Load certificate to device flash :
#endif

// sl_status_t http_client_application(void);
void https_upload_test(void);

#endif // HTTP_EVENT_H