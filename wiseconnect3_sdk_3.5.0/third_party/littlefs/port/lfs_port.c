#include "lfs.h"
#include "lfs_port.h"
#include "sfud.h"
#include "sl_constants.h"

int lfs_flash_read(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t addr = block * cfg->block_size + off;    // 计算绝对物理地址 (4K对齐)
    const sfud_flash *flash = sfud_get_device_table () + 0;

    sfud_read(flash, addr, size, (uint8_t *)buffer);
    return LFS_ERR_OK;
}

int lfs_flash_prog(const struct lfs_config *cfg, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = block * cfg->block_size + off;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    sfud_write(flash, addr, size, (uint8_t *)buffer);
    return LFS_ERR_OK;
}

int lfs_flash_erase(const struct lfs_config *cfg, lfs_block_t block)
{
    uint32_t addr = block * cfg->block_size;
    const sfud_flash *flash = sfud_get_device_table () + 0;

    sfud_erase(flash, addr, cfg->block_size);
    return LFS_ERR_OK;
}

int lfs_flash_sync(const struct lfs_config *cfg)
{
    UNUSED_PARAMETER(cfg);
    return LFS_ERR_OK;
}


