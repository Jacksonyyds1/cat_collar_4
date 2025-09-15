# WiFi OTA集成指南

## 概述

本指南展示如何在yq-catcollar-mainboard项目中集成WiFi OTA升级功能。OTA系统会在WiFi连接后自动检查AWS S3上的固件更新，并在有新版本时提供更新功能。

## 文件结构

```
yq-catcollar-mainboard/wifi_app/
├── wifi_ota_config.h              # OTA配置文件
├── wifi_ota_manager.h             # OTA管理器头文件
├── wifi_ota_manager.c             # OTA管理器实现
└── wifi_ota_integration_example.c # 集成示例代码
```

## 集成步骤

### 1. 修改app.c文件

在application函数中添加OTA初始化代码：

```c
// 在文件顶部添加包含文件
#include "wifi_app/wifi_ota_manager.h"

// 在application函数中，在WiFi初始化之后添加：
void application(void *argument)
{
    // ... 现有的WiFi初始化代码 ...

    // 在WiFi连接测试代码附近添加OTA初始化
    // 注释掉或保留原有的: wifi_connect_test();

    // 添加OTA初始化延迟任务
    // 我们将在WiFi连接后再初始化OTA

    // ... 其余代码保持不变 ...
}
```

### 2. 修改wifi_app.c文件

在WiFi连接成功后初始化OTA：

```c
// 在文件顶部添加
#include "wifi_ota_manager.h"

// 在WIFI_APP_IPCONFIG_DONE_STATE case中添加OTA初始化
case WIFI_APP_IPCONFIG_DONE_STATE: {
    wifi_app_clear_event(WIFI_APP_IPCONFIG_DONE_STATE);

    // 添加OTA初始化
    static bool ota_initialized = false;
    if (!ota_initialized) {
        // 延迟初始化OTA以确保网络完全就绪
        osDelay(5000);

        sl_status_t ota_status = ota_manager_init();
        if (ota_status == SL_STATUS_OK) {
            // 设置回调函数
            ota_set_progress_callback(ota_progress_callback_impl);
            ota_set_state_callback(ota_state_callback_impl);

            // 加载证书
            ota_status = ota_load_certificates();
            if (ota_status == SL_STATUS_OK) {
                // 启动OTA任务
                ota_status = ota_manager_start_task();
                if (ota_status == SL_STATUS_OK) {
                    ota_set_auto_check(true);
                    ota_initialized = true;
                    app_log_info("OTA system initialized successfully\r\n");
                } else {
                    app_log_error("Failed to start OTA task: 0x%lx\r\n", ota_status);
                }
            } else {
                app_log_error("Failed to load OTA certificates: 0x%lx\r\n", ota_status);
            }
        } else {
            app_log_error("Failed to initialize OTA manager: 0x%lx\r\n", ota_status);
        }
    }

    osSemaphoreRelease(wlan_thread_sem);
    LOG_PRINT("WIFI App IPCONFIG Done State\r\n");
} break;
```

### 3. 添加OTA回调函数到wifi_app.c

在文件中添加回调函数实现：

```c
// OTA进度回调函数
static void ota_progress_callback_impl(uint32_t progress, uint32_t total)
{
    static uint32_t last_reported_progress = 0;

    if (progress - last_reported_progress >= 5 || progress == total) {
        app_log_info("OTA Progress: %lu/%lu (%lu%%)\r\n",
                     progress, total, (progress * 100) / total);
        last_reported_progress = progress;
    }
}

// OTA状态变化回调函数
static void ota_state_callback_impl(ota_state_t state, ota_error_t error)
{
    const char* state_strings[] = {
        "IDLE", "CHECKING_VERSION", "DOWNLOADING", "INSTALLING", "COMPLETE", "ERROR"
    };

    if (state < sizeof(state_strings) / sizeof(state_strings[0])) {
        app_log_info("OTA State: %s\r\n", state_strings[state]);
    }

    if (error != OTA_ERROR_NONE) {
        const char* error_strings[] = {
            "NONE", "NETWORK", "DNS_RESOLVE", "HTTP_REQUEST",
            "VERSION_PARSE", "DOWNLOAD_FAILED", "INSTALL_FAILED", "TIMEOUT"
        };

        if (error < sizeof(error_strings) / sizeof(error_strings[0])) {
            app_log_error("OTA Error: %s\r\n", error_strings[error]);
        }
    }

    switch (state) {
        case OTA_STATE_COMPLETE:
            app_log_info("OTA update completed! Device will restart...\r\n");
            // 可以添加设备重启逻辑
            break;

        case OTA_STATE_ERROR:
            app_log_error("OTA update failed\r\n");
            break;

        default:
            break;
    }
}
```

### 4. 修改项目配置

#### 4.1 更新wifi_ota_config.h

根据你的AWS S3配置修改以下内容：

```c
// AWS S3存储桶配置
#define AWS_S3_BUCKET_HOST    "your-bucket-name.s3.your-region.amazonaws.com"
#define AWS_S3_REGION         "your-region"

// 固件文件配置
#define FIRMWARE_VERSION_FILE "version.txt"
#define FIRMWARE_BINARY_FILE  "catcollar-mainboard.bin"

// 当前固件版本 (在构建时定义)
#ifndef CURRENT_FIRMWARE_VERSION
#define CURRENT_FIRMWARE_VERSION "1.0.0"
#endif
```

#### 4.2 构建配置

在项目的编译器选项中添加版本定义：

```
-DCURRENT_FIRMWARE_VERSION="1.0.0"
```

#### 4.3 确保项目包含所需的SDK功能

在项目配置中确保包含：
- HTTP客户端支持
- SSL/TLS支持
- DNS客户端
- HTTP OTAF支持

### 5. AWS S3存储桶设置

#### 5.1 创建S3存储桶

1. 在AWS控制台创建S3存储桶
2. 设置适当的权限策略允许公共读取

#### 5.2 上传文件

存储桶结构示例：
```
your-bucket/
├── version.txt              # 包含版本号，如: 1.1.0
└── catcollar-mainboard.bin  # 固件二进制文件
```

version.txt内容示例：
```
1.1.0
```

#### 5.3 权限配置

设置存储桶策略允许公共读取：

```json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Sid": "PublicReadGetObject",
            "Effect": "Allow",
            "Principal": "*",
            "Action": "s3:GetObject",
            "Resource": "arn:aws:s3:::your-bucket-name/*"
        }
    ]
}
```

### 6. Shell命令集成（可选）

如果项目使用shell命令系统，可以在命令处理中添加OTA命令：

```c
// 在shell命令处理函数中添加
if (strcmp(command, "ota_check") == 0) {
    wifi_ota_check_command();
} else if (strcmp(command, "ota_update") == 0) {
    wifi_ota_update_command();
} else if (strcmp(command, "ota_status") == 0) {
    wifi_ota_status_command();
}
```

### 7. 测试流程

1. 编译并烧录固件（版本1.0.0）
2. 设备连接WiFi后会自动检查更新
3. 在S3上传新版本固件（版本1.1.0）和更新version.txt
4. 使用ota_check命令检查更新
5. 使用ota_update命令开始更新
6. 观察更新进度和状态

### 8. 注意事项

1. **网络连接**：确保设备有稳定的WiFi连接
2. **存储空间**：确保设备有足够空间下载固件
3. **证书验证**：使用HTTPS确保固件完整性
4. **版本管理**：使用语义化版本号（如1.0.0）
5. **错误处理**：实现适当的错误恢复机制
6. **功耗考虑**：大型固件下载会消耗较多电量

### 9. 调试技巧

1. 启用OTA调试日志：在wifi_ota_config.h中设置`OTA_DEBUG_ENABLE 1`
2. 检查网络连接状态
3. 验证S3存储桶访问权限
4. 确认固件文件大小和格式
5. 监控设备内存使用情况

## 总结

这个OTA系统提供了完整的固件更新解决方案，包括：
- 自动版本检查
- 安全的HTTPS下载
- 进度监控
- 错误处理
- 灵活的配置选项

按照上述步骤集成后，设备将具备自动检查和更新固件的能力，大大提高了产品的可维护性。