#include "sl_wifi.h"
#include "sl_net.h"
#include "sl_http_client.h"
#include "sl_net_dns.h"
#include <string.h>

#include "app_log.h"
#include "storage_api.h"
#include "filesystem_port.h"
#include "FreeRTOS.h"
#include "portable.h"

#include "wifi_http_event.h"

// //! Include index html page
// #include "index.html.h"
//! Include SSL CA certificate
#include "cc_cacert.pem.h"

// #define HTTPS_TEST_GET
#define HTTPS_TEST_POST

// // Hostname for the server
// char *hostname = "catcollar.intretech.com";
// char url[] = "/Device/GetDeviceInfo?UserId=594667541786693&DeviceId=589736164712501&TimeStamp=2024-08-28";

// //! HTTP resource name
// #define HTTP_URL "/index.html"

//! HTTP post data
#define HTTP_DATA "employee_name=MR.REDDY&employee_id=RSXYZ123&designation=Engineer&company=SILABS&location=Hyderabad"


// Variable to store the server IP address
static uint8_t server_ip[16];

#define HTTP_SUCCESS_RESPONSE 1
#define HTTP_FAILURE_RESPONSE 2

//! End of data indications
// No data pending from host
#define HTTP_END_OF_DATA 1

/******************************************************
 *               Function Declarations
 ******************************************************/
sl_status_t http_client_application(void);
static sl_status_t http_response_status(volatile uint8_t *response);
sl_status_t http_put_response_callback_handler(const sl_http_client_t *client,
                                               sl_http_client_event_t event,
                                               void *data,
                                               void *request_context);
sl_status_t http_get_response_callback_handler(const sl_http_client_t *client,
                                               sl_http_client_event_t event,
                                               void *data,
                                               void *request_context);
sl_status_t http_post_response_callback_handler(const sl_http_client_t *client,
                                                sl_http_client_event_t event,
                                                void *data,
                                                void *request_context);
static void reset_http_handles(void);

//! Application buffer
static uint8_t app_buffer[APP_BUFFER_LENGTH] = { 0 };

//! Application buffer index
uint32_t app_buff_index = 0;

volatile uint8_t http_rsp_received = 0;
volatile uint8_t end_of_file       = 0;
sl_status_t callback_status        = SL_STATUS_OK;


sl_status_t http_client_application(void)
{
  sl_status_t status                                  = SL_STATUS_OK;
  sl_http_client_t client_handle                      = 0;
  sl_http_client_configuration_t client_configuration = { 0 };
  sl_http_client_request_t client_request             = { 0 };

#ifdef AUTHENTICATION_ENABLE
  //! Set HTTP Client credentials
  uint16_t username_length = strlen(HTTP_CLIENT_USERNAME);
  uint16_t password_length = strlen(HTTP_CLIENT_PASSWORD);
  uint32_t credential_size = sizeof(sl_http_client_credentials_t) + username_length + password_length;

  sl_http_client_credentials_t *client_credentials = (sl_http_client_credentials_t *)malloc(credential_size);
  SL_VERIFY_POINTER_OR_RETURN(client_credentials, SL_STATUS_ALLOCATION_FAILED);
  memset(client_credentials, 0, credential_size);
  client_credentials->username_length = username_length;
  client_credentials->password_length = password_length;

  memcpy(&client_credentials->data[0], HTTP_CLIENT_USERNAME, username_length);
  memcpy(&client_credentials->data[username_length], HTTP_CLIENT_PASSWORD, password_length);

  status = sl_net_set_credential(SL_NET_HTTP_CLIENT_CREDENTIAL_ID(0),
                                 SL_NET_HTTP_CLIENT_CREDENTIAL,
                                 client_credentials,
                                 credential_size);
  if (status != SL_STATUS_OK) {
    free(client_credentials);
    return status;
  }
#else
  sl_http_client_credentials_t empty_cred = {0};
  status = sl_net_set_credential(SL_NET_HTTP_CLIENT_CREDENTIAL_ID(0),
                                SL_NET_HTTP_CLIENT_CREDENTIAL,
                                &empty_cred,
                                sizeof(empty_cred));
  if (status != SL_STATUS_OK) {
    app_log_error("Failed to set HTTP client credentials: 0x%X\r\n", status);
    return status;
  }
 #endif 

  //! Fill HTTP client_configurations
  client_configuration.network_interface = SL_NET_WIFI_CLIENT_INTERFACE;
  client_configuration.ip_version        = IP_VERSION;
  client_configuration.http_version      = HTTP_VERSION;
#if HTTPS_ENABLE
  client_configuration.https_enable      = true;
  client_configuration.tls_version       = TLS_VERSION;
  client_configuration.certificate_index = CERTIFICATE_INDEX;
#endif

  //! Fill HTTP client_request configurations
  client_request.ip_address      = (uint8_t *)server_ip;
  client_request.host_name       = (uint8_t *)CATCOLLAR_UPLOAD_HOSTNAME;
  client_request.port            = HTTP_PORT;
  client_request.resource        = (uint8_t *)CATCOLLAR_UPLOAD_URL;
  client_request.extended_header = NULL;

  status = sl_http_client_init(&client_configuration, &client_handle);
  VERIFY_STATUS_AND_RETURN(status);
  printf("\r\nHTTP Client init success\r\n");

#if EXTENDED_HEADER_ENABLE
  //! Add extended headers
  status = sl_http_client_add_header(&client_request, KEY1, VAL1);
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);

  status = sl_http_client_add_header(&client_request, KEY2, VAL2);
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);

  status = sl_http_client_add_header(&client_request, KEY3, VAL3);
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);

  status = sl_http_client_add_header(&client_request, KEY4, VAL4);
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
#endif

//   //! Configure HTTP PUT request
//   client_request.http_method_type = SL_HTTP_PUT;
//   client_request.body             = NULL;
//   client_request.body_length      = total_put_data_len;

//   //! Initialize callback method for HTTP PUT request
//   status = sl_http_client_request_init(&client_request, http_put_response_callback_handler, "This is HTTP client");
//   CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
//   printf("\r\nHTTP PUT request init success\r\n");

//   //! Send HTTP PUT request
//   status = sl_http_client_send_request(&client_handle, &client_request);
//   if (status == SL_STATUS_IN_PROGRESS) {
//     status = http_response_status(&http_rsp_received);
//     CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);
//   } else {
//     CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
//   }
// app_log_info("HTTP send request success\r\n");

//   //! Write HTTP PUT data
//   while (!end_of_file) {
//     //! Get the current length that you want to send
//     chunk_length = ((total_put_data_len - offset) > SL_HTTP_CLIENT_MAX_WRITE_BUFFER_LENGTH)
//                      ? SL_HTTP_CLIENT_MAX_WRITE_BUFFER_LENGTH
//                      : (total_put_data_len - offset);

//     if (chunk_length > 0) {
//       status = sl_http_client_write_chunked_data(&client_handle, (uint8_t *)(sl_index + offset), chunk_length, 0);

//       if (status == SL_STATUS_IN_PROGRESS) {
//         status = http_response_status(&http_rsp_received);
//         CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);

//         offset += chunk_length;
//       } else {
//         CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
//       }
//     }
//   }

//   printf("\r\nHTTP PUT request Success!\r\n");
//   reset_http_handles();

  //! Configure HTTP GET request
  client_request.http_method_type = SL_HTTP_GET;

  //! Initialize callback method for HTTP GET request
  status = sl_http_client_request_init(&client_request, http_get_response_callback_handler, "This is HTTP client");
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
  printf("\r\nHTTP Get request init success\r\n");

  //! Send HTTP GET request
  status = sl_http_client_send_request(&client_handle, &client_request);
  if (status == SL_STATUS_IN_PROGRESS) {
    status = http_response_status(&http_rsp_received);
    CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);
  } else {
    CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
  }

  printf("\r\nHTTP GET request Success\r\n");
  reset_http_handles();

  // //! Configure HTTP POST request
  // client_request.http_method_type = SL_HTTP_POST;
  // client_request.body             = (uint8_t *)HTTP_DATA;
  // client_request.body_length      = strlen(HTTP_DATA);

  // //! Initialize callback method for HTTP POST request
  // status = sl_http_client_request_init(&client_request, http_post_response_callback_handler, "This is HTTP client");
  // CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
  // printf("\r\nHTTP Post request init success\r\n");

  // //! Send HTTP POST request
  // status = sl_http_client_send_request(&client_handle, &client_request);
  // if (status == SL_STATUS_IN_PROGRESS) {
  //   status = http_response_status(&http_rsp_received);
  //   CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);
  // } else {
  //   CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
  // }

  // printf("\r\nHTTP POST request Success\r\n");
  // reset_http_handles();

#if EXTENDED_HEADER_ENABLE
  status = sl_http_client_delete_all_headers(&client_request);
  CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
#endif

  // status = sl_http_client_deinit(&client_handle);
  // VERIFY_STATUS_AND_RETURN(status);
  // printf("\r\nHTTP Client deinit success\r\n");
  // free(client_credentials);

  return status;
}

sl_status_t http_put_response_callback_handler(const sl_http_client_t *client,
                                               sl_http_client_event_t event,
                                               void *data,
                                               void *request_context)
{
  UNUSED_PARAMETER(client);
  UNUSED_PARAMETER(event);

  sl_http_client_response_t *put_response = (sl_http_client_response_t *)data;
  callback_status                         = put_response->status;

  app_log_debug("\r\n===========HTTP PUT RESPONSE START===========\r\n");
  app_log_debug(
    "\r\n> Status: 0x%X\r\n> PUT response: %u\r\n> End of data: %lu\r\n> Data Length: %u\r\n> Request Context: %s\r\n",
    put_response->status,
    put_response->http_response_code,
    put_response->end_of_data,
    put_response->data_length,
    (char *)request_context);

  if (put_response->status != SL_STATUS_OK) {
    http_rsp_received = 2;
    return put_response->status;
  }

  if (put_response->data_length) {
    if (APP_BUFFER_LENGTH > (app_buff_index + put_response->data_length)) {
      memcpy(app_buffer + app_buff_index, put_response->data_buffer, put_response->data_length);
    }
    app_buff_index += put_response->data_length;
  }
  http_rsp_received = HTTP_SUCCESS_RESPONSE;

  if (put_response->end_of_data & HTTP_END_OF_DATA) {
    end_of_file = HTTP_SUCCESS_RESPONSE;
  }

  return SL_STATUS_OK;
}

sl_status_t http_get_response_callback_handler(const sl_http_client_t *client,
                                               sl_http_client_event_t event,
                                               void *data,
                                               void *request_context)
{
  UNUSED_PARAMETER(client);
  UNUSED_PARAMETER(event);

  sl_http_client_response_t *get_response = (sl_http_client_response_t *)data;
  callback_status                         = get_response->status;

  app_log_debug("\r\n===========HTTP GET RESPONSE START===========\r\n");
  app_log_debug(
    "\r\n> Status: 0x%X\r\n> GET response: %u\r\n> End of data: %lu\r\n> Data Length: %u\r\n> Request Context: %s\r\n",
    get_response->status,
    get_response->http_response_code,
    get_response->end_of_data,
    get_response->data_length,
    (char *)request_context);

  // if (get_response->status != SL_STATUS_OK
  //     || (get_response->http_response_code >= 400 && get_response->http_response_code <= 599
  //         && get_response->http_response_code != 0)) {
  //   http_rsp_received = HTTP_FAILURE_RESPONSE;
  //   callback_status   = SL_STATUS_FAIL;
  //   return get_response->status;
  // }

  if (get_response->data_length) {
    if (APP_BUFFER_LENGTH > (app_buff_index + get_response->data_length)) {
      memcpy(app_buffer + app_buff_index, get_response->data_buffer, get_response->data_length);
    }
    app_buff_index += get_response->data_length;
  }

  if (get_response->end_of_data) {
    http_rsp_received = HTTP_SUCCESS_RESPONSE;
  }
  
  app_log_debug("\r\nGET Data response:\r\n%s \r\nOffset: %ld\r\n", app_buffer, app_buff_index);
  app_buff_index = 0;

  return SL_STATUS_OK;
}

sl_status_t http_post_response_callback_handler(const sl_http_client_t *client,
                                                sl_http_client_event_t event,
                                                void *data,
                                                void *request_context)
{
  UNUSED_PARAMETER(client);
  UNUSED_PARAMETER(event);

  sl_http_client_response_t *post_response = (sl_http_client_response_t *)data;
  callback_status = post_response->status;

  app_log_debug("\r\n===========HTTP POST RESPONSE START===========\r\n");
  app_log_debug(
    "\r\n> Status: 0x%X\r\n> POST response: %u\r\n> End of data: %lu\r\n> Data Length: %u\r\n> Request Context: %s\r\n",
    post_response->status,
    post_response->http_response_code,
    post_response->end_of_data,
    post_response->data_length,
    (char *)request_context);

  if (post_response->status != SL_STATUS_OK
      || (post_response->http_response_code >= 400 && post_response->http_response_code <= 599
          && post_response->http_response_code != 0)) {
    http_rsp_received = HTTP_FAILURE_RESPONSE;
    return post_response->status;
  }

  if (post_response->data_length) {
    if (APP_BUFFER_LENGTH > (app_buff_index + post_response->data_length)) {
      memcpy(app_buffer + app_buff_index, post_response->data_buffer, post_response->data_length);
    }
    app_buff_index += post_response->data_length;
  }

  if (post_response->end_of_data) {
    http_rsp_received = HTTP_SUCCESS_RESPONSE;
  }
  
  app_log_debug("\r\nGET Data response:\r\n%s \r\nOffset: %ld\r\n", app_buffer, app_buff_index);

  return SL_STATUS_OK;
}

static sl_status_t http_response_status(volatile uint8_t *response)
{
  while (!(*response)) {
    /* Wait till response arrives */
  }

  if (*response != HTTP_SUCCESS_RESPONSE) {
    return SL_STATUS_FAIL;
  }

  // Reset response
  *response = 0;

  return SL_STATUS_OK;
}

static void reset_http_handles(void)
{
  app_buff_index = 0;
  end_of_file    = 0;
}

// Function the resolve the IP address based on the hostname parameter
static sl_status_t sl_resolve_IP(char *hostname, uint8_t *server_ip)
{
  sl_status_t status = SL_STATUS_OK;

  sl_ip_address_t dns_query_rsp = { 0 };
  uint32_t server_address;
  int32_t dns_retry_count = MAX_DNS_RETRY_COUNT;

  do
  {
    //! Getting IP address of the remote server using DNS request
    status = sl_net_dns_resolve_hostname((const char *)hostname,
                                         DNS_TIMEOUT,
                                         SL_NET_DNS_TYPE_IPV4,
                                         &dns_query_rsp);
    dns_retry_count--;
  } while ((dns_retry_count != 0) && (status != SL_STATUS_OK));

  if (status != SL_STATUS_OK) {
    return status;
  }

  server_address = dns_query_rsp.ip.v4.value;
  sprintf((char *)server_ip,
          "%ld.%ld.%ld.%ld",
          server_address & 0x000000ff,
          (server_address & 0x0000ff00) >> 8,
          (server_address & 0x00ff0000) >> 16,
          (server_address & 0xff000000) >> 24);

  printf("Server IP address = %s\r\n", server_ip);

  return status;
}

static sl_status_t https_server_init(char *hostname)
{
  sl_status_t status = SL_STATUS_OK;
#if HTTPS_ENABLE && LOAD_CERTIFICATE
  // Load SSL CA certificate
  status = sl_net_set_credential(SL_NET_TLS_SERVER_CREDENTIAL_ID(CERTIFICATE_INDEX),
                                 SL_NET_SIGNING_CERTIFICATE,
                                 cc_cacert,
                                 sizeof(cc_cacert) - 1);
  if (status != SL_STATUS_OK) {
    app_log_error("\r\nLoading TLS CA certificate in to FLASH Failed, Error Code : 0x%lX\r\n", status);
    return status;
  }
  app_log_debug("\r\nLoad TLS CA certificate at index %d Success\r\n", CERTIFICATE_INDEX);
#endif

  status = sl_resolve_IP(hostname, server_ip);
  if (status != SL_STATUS_OK) {
    app_log_error("Unexpected error while resolving dns, Error 0x%lX\r\n", status);
    return status;
  }

  return status;
}

#define UPLOAD_USE_FS
static sl_status_t https_upload_file(const char *file_path)
{
  // (void)file_path;
  sl_status_t status                              = SL_STATUS_OK;
  sl_http_client_t client_handle                  = 0;
  sl_http_client_configuration_t client_config    = { 0 };
  sl_http_client_request_t upload_client_request  = { 0 };
   
   app_log_info("Init Free heap size: %ld bytes\r\n", xPortGetFreeHeapSize());

#ifdef UPLOAD_USE_FS
  int ret = 0;
  size_t bytes_remaining = 0;
  size_t chunk_size = 0;
  fs_file_t file;
  char fname[MAX_PATH_LEN];
  snprintf(fname, sizeof(fname), "%s/%s.imu", fs_mount_cat.mnt_point, file_path);
  fs_status file_stat;
  ret = fs_stat(fname, &file_stat);
  if (ret < 0) {
    app_log_error("FAIL: stat %s: %d\r\n", fname, ret);
    return ret;
  }
  size_t file_size = file_stat.size;
  bytes_remaining = file_size;

  ret = fs_open(&file, fname, FS_O_READ);
  if (ret < 0) {
    app_log_error("FAIL: open %s: %d\r\n", fname, ret);
    return ret;
  }
  
  // uint8_t *file_data = pvPortMalloc(file_size);
  // if (!file_data) {
  //   app_log_error("FAIL: malloc %d bytes\r\n", file_size);
  //   fs_close(&file);
  //   return SL_STATUS_ALLOCATION_FAILED;
  // }
  // ret = fs_read(&file, file_data, file_size);
  // if (ret >= 0) {
  //     app_log_info("read %d bytes from %s\r\n", file_size, fname);
  // } else if (ret < 0 || (unsigned int)ret < file_size) {
  //     app_log_error("FAIL: read %s: [rd:%d]\r\n", fname, ret);
  //     vPortFree(file_data);
  //     fs_close(&file);
  //     return SL_STATUS_FAIL;
  // }
  // fs_close(&file);
#endif

    //! Set HTTP Client credentials
    sl_http_client_credentials_t empty_cred = {0};
    status = sl_net_set_credential(SL_NET_HTTP_CLIENT_CREDENTIAL_ID(0),
                                  SL_NET_HTTP_CLIENT_CREDENTIAL,
                                  &empty_cred,
                                  sizeof(empty_cred));
    if (status != SL_STATUS_OK) {
      app_log_error("Failed to set HTTP client credentials: 0x%X\r\n", status);
      return status;
    }
app_log_info("Init Free heap size-1: %ld bytes\r\n", xPortGetFreeHeapSize());
    //! Fill HTTP client_configurations
    client_config.network_interface   = SL_NET_WIFI_CLIENT_INTERFACE;
    client_config.ip_version          = IP_VERSION;
    client_config.http_version        = HTTP_VERSION;
#if HTTPS_ENABLE    
    client_config.https_enable        = true; // 启用HTTPS
    client_config.tls_version         = TLS_VERSION;
    client_config.certificate_index   = CERTIFICATE_INDEX;
#endif

    //! Fill HTTP upload_client_request configurations
    upload_client_request.ip_address      = (uint8_t *)server_ip;
    upload_client_request.host_name       = (uint8_t *)CATCOLLAR_UPLOAD_HOSTNAME;
    upload_client_request.port            = HTTP_PORT;
    upload_client_request.resource        = (uint8_t *)CATCOLLAR_UPLOAD_URL;
    upload_client_request.extended_header = NULL;

    status = sl_http_client_init(&client_config, &client_handle);
    VERIFY_STATUS_AND_RETURN(status);
    app_log_info("HTTP client initialized...\r\n");
app_log_info("Init Free heap size-2: %ld bytes\r\n", xPortGetFreeHeapSize());
#ifdef HTTPS_TEST_GET
    upload_client_request.http_method_type = SL_HTTP_GET;
    status = sl_http_client_request_init(&upload_client_request, http_get_response_callback_handler, "This is HTTP client");
#endif

#ifdef HTTPS_TEST_POST
    //! Configure HTTP POST request
    upload_client_request.http_method_type = SL_HTTP_POST;
    // upload_client_request.body             = (uint8_t *)HTTP_DATA;
    // upload_client_request.body_length      = strlen(HTTP_DATA);
    upload_client_request.body             = NULL;
    upload_client_request.body_length      = file_size;

    //! Initialize callback method for HTTP POST request
    status = sl_http_client_request_init(&upload_client_request, http_post_response_callback_handler, "File upload");
#endif
    /// TODO: Add free for file_data!
    CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
app_log_info("HTTPS %s request init success\r\n", upload_client_request.http_method_type == SL_HTTP_GET ? "Get" : "Post");

    //! Send HTTP Server request
    status = sl_http_client_send_request(&client_handle, &upload_client_request);
app_log_info("Init Free heap size-3: %ld bytes\r\n", xPortGetFreeHeapSize());

  // //! Write HTTP PUT data
  // while (!http_rsp_received) {

  //   //! Get the current length that you want to send
  //   chunk_length = ((total_put_data_len - offset) > SL_HTTP_CLIENT_MAX_WRITE_BUFFER_LENGTH)
  //                    ? SL_HTTP_CLIENT_MAX_WRITE_BUFFER_LENGTH
  //                    : (total_put_data_len - offset);

  //   if (chunk_length > 0) {
  //     status = sl_http_client_write_chunked_data(&client_handle, (uint8_t *)(sl_index + offset), chunk_length, 0);

  //     if (status == SL_STATUS_IN_PROGRESS) {
  //       status = http_response_status(&http_rsp_received);
  //       CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);

  //       offset += chunk_length;
  //     } else {
  //       CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
  //     }
  //   }
  // }


    uint8_t buffer[800];  // 每个块最大800字节
    size_t total_sent = 0;
    bool upload_complete = false;
    bool upload_failed = false;

    while (!upload_complete && !upload_failed) {
        // 计算本次要发送的块大小
        chunk_size = bytes_remaining > sizeof(buffer) ? sizeof(buffer) : bytes_remaining;
        
        // 从文件读取数据块
        ret = fs_read(&file, buffer, chunk_size);
        if (ret < 0 || (size_t)ret != chunk_size) {
            app_log_error("File read error: expected %u, got %d\r\n", chunk_size, ret);
            upload_failed = true;
            break;
        }
app_log_info("Init Free heap size-t: %ld bytes\r\n", xPortGetFreeHeapSize());
        // 发送数据块
        status = sl_http_client_write_chunked_data(&client_handle, buffer, chunk_size, 0);
        if (status == SL_STATUS_IN_PROGRESS) {
            // 等待操作完成
            status = http_response_status(&http_rsp_received);
            if (status != SL_STATUS_OK) {
                app_log_error("HTTP write failed: 0x%X\r\n", status);
                upload_failed = true;
                break;
            }
            
            // 更新进度
            total_sent += chunk_size;
            bytes_remaining -= chunk_size;
            app_log_info("Uploaded %u/%u bytes (%.1f%%)\r\n", 
                         total_sent, file_size, 
                         (float)total_sent/file_size * 100);
            
            // 检查是否完成
            if (bytes_remaining == 0) {
                upload_complete = true;
            }
        } else if (status != SL_STATUS_OK) {
            app_log_error("HTTP write error: 0x%X\r\n", status);
            upload_failed = true;
            break;
        }
    }

    // 检查最终状态
    if (upload_complete) {
        // 通知文件传输完成
        status = sl_http_client_write_chunked_data(&client_handle, NULL, 0, 1);
        if (status == SL_STATUS_IN_PROGRESS) {
            status = http_response_status(&http_rsp_received);
        }
        
        if (status == SL_STATUS_OK) {
            app_log_info("File upload completed successfully\r\n");
        } else {
            app_log_error("Finalize upload failed: 0x%X\r\n", status);
        }
    } else if (upload_failed) {
        app_log_error("File upload failed\r\n");
        status = SL_STATUS_FAIL;
    }




    // if (status == SL_STATUS_IN_PROGRESS) {
    //   status = http_response_status(&http_rsp_received);
    //   CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_ASYNC_RESPONSE);
    // } else {
    //   CLEAN_HTTP_CLIENT_IF_FAILED(status, &client_handle, HTTP_SYNC_RESPONSE);
    // }

    app_log_info("HTTPS %s request Success\r\n", upload_client_request.http_method_type == SL_HTTP_GET ? "Get" : "Post");
   
    fs_close(&file);
    status = sl_http_client_deinit(&client_handle);
    reset_http_handles();
    VERIFY_STATUS_AND_RETURN(status);
    printf("\r\nHTTP Client deinit success\r\n");
    return status;
}

void https_upload_test(void)
{
    sl_status_t status  = SL_STATUS_OK;

    status = https_server_init(CATCOLLAR_UPLOAD_HOSTNAME);
    if (status != SL_STATUS_OK) {
        app_log_error("Failed to initialize HTTPS server: 0x%X\r\n", status);
        return;
    }

    status = https_upload_file("active1");
    // status = http_client_application();
    if (status != SL_STATUS_OK) {
        printf("\r\nUnexpected error while HTTP client operation: 0x%lX\r\n", status);
        return;
    }

  printf("\r\nApplication Demonstration Completed Successfully!\r\n");
}

