/*
 * lfs_port.h
 *
 *  Created on: 2025年7月28日
 *      Author: YQ05165
 */

#ifndef LFS_PORT_H_
#define LFS_PORT_H_

#include "lfs.h"

#define FLASH_BLOCK_SIZE    4096

int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block);
int lfs_flash_sync(const struct lfs_config *cfg);

// static const struct lfs_config flash_cfg = {
// 	.read = lfs_flash_read,
// 	.prog = lfs_flash_prog,
// 	.erase = lfs_flash_erase,
// 	.sync = lfs_flash_sync,
	
//     .read_size = 1,     //单字节读取
//     .prog_size = 256,   //页编程粒度 = 256 字节
//     .block_size = FLASH_BLOCK_SIZE,     //块大小 = 4096 字节
//     .block_count = 4096,                //总块数 = 16MB/4kB = 4096
//     .cache_size = 512,
//     .lookahead_size = 512,
//     .block_cycles = 500,
// };

#endif /* LFS_PORT_H_ */
