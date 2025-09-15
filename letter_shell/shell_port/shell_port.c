/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#include "cmsis_os2.h"
#include "sl_utility.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "time.h"

#include "shell.h"
//#include "serial.h"
// #include "stm32f4xx_hal.h"
// #include "usart.h"
#include "console.h"
#include "rsi_debug.h"
//#include "cevent.h"
#include "app_log.h"

// sl_usart_handle_t usart_handle;
// #define USART_BAUDRATE    921600 // Baud rate <9600-7372800>
extern int _write(int file, char *ptr, int len);

void userShellInit(void);

Shell shell;
char shellBuffer[512];

static SemaphoreHandle_t shellMutex;

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际写入的数据长度
 */
short userShellWrite(char *data, unsigned short len)
{
    // sl_status_t status;
    // serialTransmit(&debugSerial, (uint8_t *)data, len, 0x1FF);
    // printf("%s", data);
    // return len;
    return (short)_write(1, data, len);
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际读取到
 */
short userShellRead(char *data, unsigned short len)
{
    unsigned short count = 0;
    
    // 确保有数据可读
    if (!console_data_rx_receive) {
        return 0;
    }
    
    // 从console缓存读取数据
    count = (unsigned short)console_read_data_from_cache(data, len);
    return count;
}

/**
 * @brief 用户shell上锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellLock(Shell *shell)
{
    (void)shell;
    xSemaphoreTakeRecursive(shellMutex, portMAX_DELAY);
    return 0;
}

/**
 * @brief 用户shell解锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellUnlock(Shell *shell)
{
    (void)shell;
    xSemaphoreGiveRecursive(shellMutex);
    return 0;
}

/**
 * @brief 用户shell初始化
 * 
 */
#define SHELL_TASK_STACK_SIZE   512
static StackType_t shell_task_TaskStack[SHELL_TASK_STACK_SIZE];
static StaticTask_t shell_task_TaskHandle;
void userShellInit(void)
{
    shellMutex = xSemaphoreCreateMutex();

    shell.write = userShellWrite;
    shell.read = userShellRead;
    shell.lock = userShellLock;
    shell.unlock = userShellUnlock;
    shellInit(&shell, shellBuffer, 512);
    
    if (xTaskCreateStatic(
        shellTask,           // 任务函数
        "shellTask",         // 任务名称
        SHELL_TASK_STACK_SIZE,    // 堆栈大小
        &shell,                   // 任务参数
        osPriorityNormal6,         // 任务优先级 configMAX_PRIORITIES
        shell_task_TaskStack,      // 静态任务堆栈
        &shell_task_TaskHandle     // 静态任务控制块
    ) != NULL){
        app_log_info("shell task create success\r\n");
    } else{
        app_log_error("shell task create failed\r\n");
    }

    // if (xTaskCreate(shellTask, "shell", 256, &shell, 5, NULL) != pdPASS)
    // {
    //     app_log_error("shell task creat failed\r\n");
    // }
}

void shellPortInit(void)
{
    // sl_status_t status;
    // sl_si91x_usart_control_config_t usart_config;

    // usart_config.baudrate      = USART_BAUDRATE;
    // usart_config.mode          = SL_USART_MODE_ASYNCHRONOUS;
    // usart_config.parity        = SL_USART_NO_PARITY;
    // usart_config.stopbits      = SL_USART_STOP_BITS_1;
    // usart_config.hwflowcontrol = SL_USART_FLOW_CONTROL_NONE;
    // usart_config.databits      = SL_USART_DATA_BITS_8;
    // usart_config.misc_control  = SL_USART_MISC_CONTROL_NONE;
    // usart_config.usart_module  = USART_0;
    // usart_config.config_enable = ENABLE;
    // usart_config.synch_mode    = DISABLE;

    // // Initialize the UART
    // status = sl_si91x_usart_init(USART_0, &usart_handle);
    // if (status != SL_STATUS_OK) {
    //   app_log_error("sl_si91x_usart_initialize: Error Code : %lu \r\n", status);
    // }

    // // Configure the USART configurations
    // status = sl_si91x_usart_set_configuration(usart_handle, &usart_config);
    // if (status != SL_STATUS_OK) {
    //   app_log_error("sl_si91x_usart_set_configuration: Error Code : %lu \r\n", status);
    // }

    app_log_info("shell port init success\r\n");

    userShellInit();
}

