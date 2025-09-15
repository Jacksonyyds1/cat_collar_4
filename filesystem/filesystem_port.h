#ifndef FILESYSTEM_PORT_H
#define FILESYSTEM_PORT_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 文件模式定义
typedef enum fs_mode {
    FS_O_READ     = 0x01,   // 读模式
    FS_O_WRITE    = 0x02,   // 写模式
    FS_O_RDWR     = 0x03,   // 读写模式
    FS_O_CREATE   = 0x04,   // 创建文件
    FS_O_APPEND   = 0x08,   // 追加模式
    FS_O_TRUNC    = 0x10,   // 截断文件
} fs_mode_t;

// 文件定位定义
typedef enum fs_whence {
    FS_SEEK_SET = 0,  // 文件开头
    FS_SEEK_CUR = 1,  // 当前位置
    FS_SEEK_END = 2,  // 文件结尾
} fs_whence_t;

// 文件类型定义
typedef enum fs_file_type {
    FS_DIR_ENTRY_FILE = 0,  // 文件
    FS_DIR_ENTRY_DIR = 1,   // 目录
} fs_file_type_t;

// // 文件状态结构
// struct fs_stat {
//     char name[256];          // 文件名
//     off_t size;              // 文件大小
//     fs_file_type_t type;     // 文件类型
// };

// 文件、目录项结构
typedef struct  {
    char name[256];          // 文件名
    size_t size;             // 文件大小
    fs_file_type_t type;     // 文件类型
}fs_dirent, fs_status;

// struct fs_dir_t {
//     lfs_dir_t lfs_dir;  // LittleFS内部目录结构
//     char path[256];     // 当前目录路径
//     uint8_t is_open;    // 目录是否已打开
// };

// 文件描述符
typedef struct {
    void* file_ptr;          // 指向 LittleFS 文件对象
} fs_file_t;

// 目录描述符
typedef struct {
    void* dir_ptr;           // 指向 LittleFS 目录对象
} fs_dir_t;

// 挂载点配置
typedef struct {
    const char* mnt_point;   // 挂载点路径
    void* fs_data;           // 文件系统私有数据
    void* storage_dev;       // 存储设备句柄
} fs_mount_t;

// 文件系统统计信息
struct fs_statvfs {
    unsigned long f_bsize;   // 块大小
    unsigned long f_frsize;  // 基础块大小
    unsigned long f_blocks;  // 总块数
    unsigned long f_bfree;   // 空闲块数
};

void fs_init(void);

// ========== 核心 API 接口 ==========
int fs_open(fs_file_t* file, const char* path, fs_mode_t mode);
int fs_open_ts(fs_file_t* file, const char* path, fs_mode_t mode);
int fs_close(fs_file_t* file);
int fs_close_ts(fs_file_t* file);
ssize_t fs_read(fs_file_t* file, void* buf, size_t len);
// ssize_t fs_read_ts(fs_file_t* file, void* buf, size_t len);
ssize_t fs_write(fs_file_t* file, const void* buf, size_t len);
ssize_t fs_write_ts(fs_file_t* file, const void* buf, size_t len);
int fs_seek(fs_file_t* file, off_t offset, fs_whence_t whence);
off_t fs_tell(fs_file_t* file);
int fs_rewind(fs_file_t* file);
int fs_stat(const char* path, fs_status* stat);
int fs_mkdir(const char* path);
int fs_opendir(fs_dir_t* dir, const char* path);
int fs_readdir(fs_dir_t* dir, fs_dirent* entry);
int fs_closedir(fs_dir_t* dir);
void fs_dir_t_init(fs_dir_t *dir);
void fs_file_t_init(fs_file_t* file);
int fs_mount(fs_mount_t* mp);
int fs_unmount(fs_mount_t* mp);

// ========== 扩展功能 ==========
int fs_rename(const char* old_path, const char* new_path);
int fs_unlink(const char* path);
int fs_truncate(fs_file_t* file, off_t length);
int fs_sync(fs_file_t* file);
int fs_statvfs(struct fs_statvfs* stat);


#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_PORT_H