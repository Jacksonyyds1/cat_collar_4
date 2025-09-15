// wifi_ota_config.h 的改进建议

#ifndef WIFI_OTA_CONFIG_H
#define WIFI_OTA_CONFIG_H

#include <stdint.h>

// Macro to set specified bit position
#ifndef BIT
#define BIT(a) ((uint32_t)1U << a)
#endif

/*==============================================*/
/**
 * OTA Configuration
 */

// 固件更新类型
#define M4_FW_UPDATE       0
#define TA_FW_UPDATE       1
#define COMBINED_FW_UPDATE 2

// 设置固件更新类型
#define FW_UPDATE_TYPE  M4_FW_UPDATE

// 证书配置
#define LOAD_CERTIFICATE 1
#define USE_SDK_AWS_CERTIFICATE 1

// HTTP标志位定义
#define HTTPS_SUPPORT BIT(0)
#define HTTPV6 BIT(3)
#define HTTP_POST_DATA BIT(5)
#define HTTP_V_1_1 BIT(6)
#define HTTP_USER_DEFINED_CONTENT_TYPE BIT(7)

// 证书索引 (确保与加载的证书索引一致) - 使用OTA专用的宏名避免冲突
#define OTA_CERTIFICATE_INDEX 0

/*==============================================*/
/**
 * AWS S3配置
 */

// AWS S3存储桶配置
#define AWS_S3_BUCKET_HOST    "cat-firmware-bucket-001.s3.us-east-2.amazonaws.com"
#define AWS_S3_REGION         "us-east-2"

// 固件文件配置
#define FIRMWARE_VERSION_FILE "firmware/version.txt"
#define FIRMWARE_BINARY_FILE  "firmware/yq-catcollar-mainboard.bin"

// HTTP配置 - 针对错误 0x1bb49 的修复尝试
// 选项1: 启用HTTPS (需要正确的证书)
#define OTA_FLAGS_HTTPS   (HTTPS_SUPPORT | HTTP_V_1_1)
#define OTA_HTTP_PORT_HTTPS 443

// 选项2: 临时使用HTTP进行调试 (如果S3支持)
#define OTA_FLAGS_HTTP    (HTTP_V_1_1)
#define OTA_HTTP_PORT_HTTP 80

// 当前使用的配置 - 可以在调试时切换
#define OTA_FLAGS       OTA_FLAGS_HTTPS
#define OTA_HTTP_PORT   OTA_HTTP_PORT_HTTPS

// 超时配置 - 使用OTA专用的宏名避免冲突
#define OTA_TIMEOUT         1200000  // 20分钟
#define OTA_DNS_TIMEOUT     10000    // 10秒
#define OTA_MAX_DNS_RETRY_COUNT 3

// 版本检查间隔 (秒)
#define VERSION_CHECK_INTERVAL 3600  // 1小时检查一次

/*==============================================*/
/**
 * 版本管理
 */

// 当前固件版本 (在构建时定义)
#ifndef CURRENT_FIRMWARE_VERSION
#define CURRENT_FIRMWARE_VERSION "1.0.0"
#endif

// 版本字符串最大长度
#define MAX_VERSION_STRING_LENGTH 32

/*==============================================*/
/**
 * HTTP扩展头和认证
 */

// 用于AWS S3访问的HTTP扩展头
#define HTTP_EXTENDED_HEADER NULL

// S3公共访问不需要用户名密码
#define OTA_USERNAME ""
#define OTA_PASSWORD ""

/*==============================================*/
/**
 * 调试配置
 */

// 启用OTA调试打印
#define OTA_DEBUG_ENABLE 1

#if OTA_DEBUG_ENABLE
#define OTA_LOG_INFO(...)  do { \
    printf("[OTA INFO] " __VA_ARGS__); \
} while(0)

#define OTA_LOG_ERROR(...) do { \
    printf("[OTA ERROR] " __VA_ARGS__); \
} while(0)

#define OTA_LOG_DEBUG(...) do { \
    printf("[OTA DEBUG] " __VA_ARGS__); \
} while(0)
#else
#define OTA_LOG_INFO(...)
#define OTA_LOG_ERROR(...)
#define OTA_LOG_DEBUG(...)
#endif

/*==============================================*/
/**
 * 证书相关配置
 */

// 如果使用HTTPS，确保包含正确的AWS证书
#ifdef HTTPS_SUPPORT

// AWS使用的根证书选项：
// 1. Amazon Root CA 1 (推荐)
// 2. Starfield Services Root Certificate Authority - G2
// 3. DigiCert Global Root CA

// 确保在 ota_load_certificates() 中加载了正确的证书
extern const unsigned char aws_root_ca_1[];        // Amazon Root CA 1
extern const unsigned char aws_starfield_ca[];     // Starfield CA (当前使用)

// 证书选择 - 可以尝试不同的证书
#define USE_AMAZON_ROOT_CA_1    0  // 设置为1使用Amazon Root CA 1
#define USE_STARFIELD_CA        1  // 设置为1使用Starfield CA

#endif // HTTPS_SUPPORT

/*==============================================*/
/**
 * 错误处理配置
 */

// 下载重试次数
#define OTA_DOWNLOAD_RETRY_COUNT 3

// 重试间隔 (毫秒)
#define OTA_RETRY_DELAY_MS 5000

// 网络错误自动重试
#define OTA_AUTO_RETRY_ON_NETWORK_ERROR 1

#endif // WIFI_OTA_CONFIG_H