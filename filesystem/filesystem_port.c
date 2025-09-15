#include "filesystem_port.h"
#include "lfs.h"
#include "lfs_port.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <string.h>
#include <errno.h>
#include "app_log.h"

// LittleFS 实例
static lfs_t lfs;
static struct lfs_config lfs_cfg;

// 文件系统互斥锁
static SemaphoreHandle_t fs_mutex;

// 错误码映射
static int map_lfs_error(int lfs_error) 
{
    switch (lfs_error) {
        case LFS_ERR_OK:      return 0;
        case LFS_ERR_IO:      return -EIO;
        case LFS_ERR_CORRUPT: return -EILSEQ;
        case LFS_ERR_NOENT:   return -ENOENT;
        case LFS_ERR_EXIST:   return -EEXIST;
        case LFS_ERR_NOTDIR:  return -ENOTDIR;
        case LFS_ERR_ISDIR:   return -EISDIR;
        case LFS_ERR_NOTEMPTY:return -ENOTEMPTY;
        case LFS_ERR_BADF:    return -EBADF;
        case LFS_ERR_FBIG:    return -EFBIG;
        case LFS_ERR_INVAL:   return -EINVAL;
        case LFS_ERR_NOSPC:   return -ENOSPC;
        case LFS_ERR_NOMEM:   return -ENOMEM;
        default:              return -EIO;
    }
}

// 转换文件打开模式
static int map_fs_mode(fs_mode_t mode) 
{
    int lfs_flags = 0;
    
    if (mode & FS_O_READ) lfs_flags |= LFS_O_RDONLY;
    if (mode & FS_O_WRITE) lfs_flags |= LFS_O_WRONLY;
    if (mode & FS_O_RDWR) lfs_flags |= LFS_O_RDWR;
    if (mode & FS_O_CREATE) lfs_flags |= LFS_O_CREAT;
    if (mode & FS_O_APPEND) lfs_flags |= LFS_O_APPEND;
    if (mode & FS_O_TRUNC) lfs_flags |= LFS_O_TRUNC;
    
    return lfs_flags;
}

// 初始化文件系统
void fs_init(void) 
{
    if (!fs_mutex) {
        fs_mutex = xSemaphoreCreateMutex();
        configASSERT(fs_mutex);
    }
}

int fs_open(fs_file_t* file, const char* path, fs_mode_t mode) 
{
    if (!file || !path) return -EINVAL;
    
    lfs_file_t* lfs_file = pvPortMalloc(sizeof(lfs_file_t));
    if (!lfs_file) return -ENOMEM;
    
    int flags = map_fs_mode(mode);
    int ret = lfs_file_open(&lfs, lfs_file, path, flags);
    
    if (ret == LFS_ERR_OK) {
        file->file_ptr = lfs_file;
        return 0;
    }
    
    vPortFree(lfs_file);
    return map_lfs_error(ret);
}

int fs_close(fs_file_t* file) 
{
    if (!file || !file->file_ptr) return -EBADF;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    int ret = lfs_file_close(&lfs, lfs_file);
    vPortFree(lfs_file);
    
    file->file_ptr = NULL;
    return map_lfs_error(ret);
}

ssize_t fs_read(fs_file_t* file, void* buf, size_t len) 
{
    if (!file || !file->file_ptr || !buf) return -EINVAL;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    lfs_ssize_t bytes_read = lfs_file_read(&lfs, lfs_file, buf, len);
    
    if (bytes_read < 0) return map_lfs_error(bytes_read);
    return (ssize_t)bytes_read;
}

ssize_t fs_write(fs_file_t* file, const void* buf, size_t len) 
{
    if (!file || !file->file_ptr || !buf) return -EINVAL;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    lfs_ssize_t bytes_written = lfs_file_write(&lfs, lfs_file, buf, len);
    
    if (bytes_written < 0) return map_lfs_error(bytes_written);
    return (ssize_t)bytes_written;
}

int fs_seek(fs_file_t* file, off_t offset, fs_whence_t whence) 
{
    if (!file || !file->file_ptr) return -EBADF;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    int lfs_whence;
    
    switch (whence) {
        case FS_SEEK_SET: lfs_whence = LFS_SEEK_SET; break;
        case FS_SEEK_CUR: lfs_whence = LFS_SEEK_CUR; break;
        case FS_SEEK_END: lfs_whence = LFS_SEEK_END; break;
        default: return -EINVAL;
    }
    
    int ret = lfs_file_seek(&lfs, lfs_file, offset, lfs_whence);
    if (ret != offset) {
        return map_lfs_error(ret);
    } else {
        return 0;
    }
}

off_t fs_tell(fs_file_t* file)
{
    if (!file || !file->file_ptr) return -EBADF;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    lfs_soff_t pos = lfs_file_tell(&lfs, lfs_file);
    
    if (pos < 0) return map_lfs_error(pos);
    return (off_t)pos;
}

int fs_rewind(fs_file_t* file)
{
    if (!file || !file->file_ptr) return -EBADF;

    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    int ret = lfs_file_rewind(&lfs, lfs_file);
    return map_lfs_error(ret);
}

int fs_stat(const char* path, fs_status* stat) 
{
    if (!path || !stat) return -EINVAL;
    
    struct lfs_info info;
    int ret = lfs_stat(&lfs, path, &info);
    if (ret != LFS_ERR_OK) return map_lfs_error(ret);
    
    strncpy(stat->name, info.name, sizeof(stat->name)-1);
    stat->name[sizeof(stat->name)-1] = '\0';
    stat->size = info.size;
    stat->type = (info.type == LFS_TYPE_REG) ? FS_DIR_ENTRY_FILE : FS_DIR_ENTRY_DIR;
    
    return 0;
}

int fs_mkdir(const char* path) 
{
    if (!path) return -EINVAL;
    int ret = lfs_mkdir(&lfs, path);
    return map_lfs_error(ret);
}

int fs_opendir(fs_dir_t* dir, const char* path) 
{
    if (!dir || !path) return -EINVAL;
    
    lfs_dir_t* lfs_dir = pvPortMalloc(sizeof(lfs_dir_t));
    if (!lfs_dir) return -ENOMEM;
    
    int ret = lfs_dir_open(&lfs, lfs_dir, path);
    if (ret != LFS_ERR_OK) {
        vPortFree(lfs_dir);
        return map_lfs_error(ret);
    }
    
    dir->dir_ptr = lfs_dir;
    return 0;
}

int fs_readdir(fs_dir_t* dir, fs_dirent* entry) 
{
    if (!dir || !dir->dir_ptr || !entry) return -EINVAL;
    lfs_dir_t* lfs_dir = (lfs_dir_t*)dir->dir_ptr;
    struct lfs_info info;
    
    int ret = lfs_dir_read(&lfs, lfs_dir, &info);

    if (ret < 0) return map_lfs_error(ret);
    if (ret == 0) return 0; // 目录结束
    
    strncpy(entry->name, info.name, sizeof(entry->name)-1);
    entry->name[sizeof(entry->name)-1] = '\0';
    entry->size = info.size;
    entry->type = (info.type == LFS_TYPE_REG) ? FS_DIR_ENTRY_FILE : FS_DIR_ENTRY_DIR;
    
    return 1;
}

int fs_closedir(fs_dir_t* dir) 
{
    if (!dir || !dir->dir_ptr) return -EBADF;
    
    lfs_dir_t* lfs_dir = (lfs_dir_t*)dir->dir_ptr;
    int ret = lfs_dir_close(&lfs, lfs_dir);
    vPortFree(lfs_dir);
    
    dir->dir_ptr = NULL;
    return map_lfs_error(ret);
}

int fs_mount(fs_mount_t* mp) 
{
    if (!mp) return -EINVAL;
    
    // lfs_cfg.context = mp->storage_dev;
    lfs_cfg.read  = lfs_flash_read;
    lfs_cfg.prog  = lfs_flash_prog;
    lfs_cfg.erase = lfs_flash_erase;
    lfs_cfg.sync  = lfs_flash_sync;
    
    // 性能优化参数
    lfs_cfg.read_size = 16;          //
    lfs_cfg.prog_size = 256;        //页编程粒度 = 256 字节
    lfs_cfg.block_size = FLASH_BLOCK_SIZE;      //块大小 = 4096 字节
    lfs_cfg.block_count = 4096;     //总块数 = 16MB/4kB = 4096
    lfs_cfg.cache_size = 1024;
    lfs_cfg.lookahead_size = 16;
    lfs_cfg.block_cycles = 200;

    int ret = lfs_mount(&lfs, &lfs_cfg);
    
    // 挂载失败时格式化
    if (ret != LFS_ERR_OK) {
        app_log_error("lfs_mount failed, ret=%d, format now\r\n", ret);
        ret = lfs_format(&lfs, &lfs_cfg);
        if (ret != LFS_ERR_OK) return map_lfs_error(ret);
        
        ret = lfs_mount(&lfs, &lfs_cfg);
        if (ret != LFS_ERR_OK) return map_lfs_error(ret);
    }
    
    mp->fs_data = &lfs;
    return 0;
}

int fs_unmount(fs_mount_t* mp) 
{
    if (!mp || !mp->fs_data) return -EINVAL;
    
    int ret = lfs_unmount(&lfs);
    return map_lfs_error(ret);
}

// ========== 扩展功能实现 ==========
int fs_rename(const char* old_path, const char* new_path) 
{
    if (!old_path || !new_path) return -EINVAL;
    int ret = lfs_rename(&lfs, old_path, new_path);
    return map_lfs_error(ret);
}

int fs_unlink(const char* path) 
{
    if (!path) return -EINVAL;
    int ret = lfs_remove(&lfs, path);
    return map_lfs_error(ret);
}

int fs_truncate(fs_file_t* file, off_t length) 
{
    if (!file || !file->file_ptr) return -EBADF;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    int ret = lfs_file_truncate(&lfs, lfs_file, length);
    return map_lfs_error(ret);
}

int fs_sync(fs_file_t* file) 
{
    if (!file || !file->file_ptr) return -EBADF;
    
    lfs_file_t* lfs_file = (lfs_file_t*)file->file_ptr;
    int ret = lfs_file_sync(&lfs, lfs_file);
    return map_lfs_error(ret);
}

int fs_statvfs(struct fs_statvfs* stat) 
{
    if (!stat) return -EINVAL;
    
    lfs_ssize_t free_blocks = lfs_fs_size(&lfs);
    if (free_blocks < 0) return map_lfs_error(free_blocks);
    
    stat->f_bsize = lfs_cfg.block_size;
    stat->f_frsize = lfs_cfg.block_size;
    stat->f_blocks = lfs_cfg.block_count;
    stat->f_bfree = lfs_cfg.block_count - free_blocks;
    
    return 0;
}

// ========== 线程安全封装 ==========
#define FS_LOCK() do { \
    if (fs_mutex) xSemaphoreTake(fs_mutex, portMAX_DELAY); \
} while(0)

#define FS_UNLOCK() do { \
    if (fs_mutex) xSemaphoreGive(fs_mutex); \
} while(0)

// 线程安全版本API
int fs_open_ts(fs_file_t* file, const char* path, fs_mode_t mode) 
{
    FS_LOCK();
    int ret = fs_open(file, path, mode);
    FS_UNLOCK();
    return ret;
}

int fs_close_ts(fs_file_t* file) 
{
    FS_LOCK();
    int ret = fs_close(file);
    FS_UNLOCK();
    return ret;
}

ssize_t fs_write_ts(fs_file_t* file, const void* buf, size_t len) 
{
    FS_LOCK();
    ssize_t ret = fs_write(file, buf, len);
    FS_UNLOCK();
    return ret;
}

void fs_dir_t_init(fs_dir_t *dirp)
{
    memset(&dirp, 0, sizeof(fs_dir_t));
}

void fs_file_t_init(fs_file_t* file) 
{
    memset(&file, 0, sizeof(fs_file_t)); // 清零结构体
    // file->file_ptr = NULL; // 文件指针置空
}

