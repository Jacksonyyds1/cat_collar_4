#ifndef _SYSCFG_ID_H
#define _SYSCFG_ID_H

#include <stdint.h>

///=============================================================================
// #define	VM__CONFIG_ADDR	    			2
#define VM_SYS_CONFIG_ADDR                  3
#define	VM_FW_INFO_ADDR						4
#define VM_NET_CHINESE_SSID_IMG_ADDR		5 
#define VM_NET_CHINESE_SSID_IMG_SIZE        6 
#define VM_TEST_MODE_FLAG_ADDR				7
/* --------------------------------------------------------------------------*/
/**
 * @brief 读取对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [out] buf 用于存储read数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于read数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_read(uint16_t item_id, void *buf, uint16_t len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 写入对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储write数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于write数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_write(uint16_t item_id, void *buf, uint16_t len);
///=============================================================================

int syscfg_init(void);
void restore_syscfg_config(void);

#endif
