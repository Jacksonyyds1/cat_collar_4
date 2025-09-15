#include "lfs.h"
#include "lfs_util.h"
#include "filesystem_port.h"
#include "storage_api.h"
#include "syscfg_id.h"
#include "errno.h"

#include "common.h"


static int lsdir(const char *path);
static void erase_and_reboot(void);
// int delete_all_files();
static int delete_all_files_in_mount_point(const char *mount_point);

struct file_data_t {
	char imu_filename[MAX_PATH_LEN];
	char activity_filename[MAX_PATH_LEN];
	fs_file_t imu_file;      // .imu
	fs_file_t activity_file; // .act
	uint16_t imu_sample_count;
	uint16_t activity_sample_count;
};
static struct file_data_t m_file_data[MAX_OPEN_FILES];
static int m_open_file_count = 0;

#define FILESYSTEM_CREATE_TEST_FILES 1

fs_mount_t fs_mount_cat = {
    .mnt_point = MOUNT_POINT_CAT,
    .fs_data = NULL,
    .storage_dev = NULL,
};

static int create_dir_if_not_exists(const char *path)
{
    fs_status stat;
    int ret = fs_stat(path, &stat);
    
    if (ret == 0 && stat.type == FS_DIR_ENTRY_DIR) {
        return 0;
    }
    
    // 目录不存在，尝试创建
    if (ret != 0) {
        ret = fs_mkdir(path);
        if (ret != 0 && ret != -EEXIST) {
            app_log_error("Failed to create directory %s: %d\r\n", path, ret);
            return ret;
        }
    }
    return 0;
}

int storage_init(void)
{
    int ret = 0;

    fs_init();

    // 挂载文件系统
    if (fs_mount(&fs_mount_cat) != 0) {
        app_log_error("Failed to mount filesystem\r\n");
        return -1;
    }
    
    struct fs_statvfs sbuf;
    ret = fs_statvfs(&sbuf);
    if (ret != 0) {
        app_log_error("Failed to get filesystem statistics: %d\r\n", ret);
        return ret;
    }

    app_log_info("Total space: %lu kB\r\n", sbuf.f_blocks * sbuf.f_frsize / 1024);
    app_log_info("Free space: %lu kB\r\n", sbuf.f_bfree * sbuf.f_frsize / 1024);
    double used_percent = (sbuf.f_blocks - sbuf.f_bfree) * 100.0 / sbuf.f_blocks;
    app_log_info("Storage space used: %.2f%%\r\n", used_percent);
    // app_log_info("Storage space used: %lu%%\r\n", (sbuf.f_blocks - sbuf.f_bfree) * 100 / sbuf.f_blocks);

    create_dir_if_not_exists(fs_mount_cat.mnt_point);

    syscfg_init();

#if FILESYSTEM_CREATE_TEST_FILES

    create_dir_if_not_exists(PATH_CAT("config"));
    create_dir_if_not_exists(PATH_CAT("logs"));
    create_dir_if_not_exists(PATH_CAT("data"));
    create_dir_if_not_exists(PATH_CAT("data/images"));
    
    // 根目录文件
    fs_file_t root_file;
    if (fs_open(&root_file, PATH_CAT("root_file.txt"), FS_O_CREATE | FS_O_WRITE) == 0) {
        const char* root_data = "This is a root level file";
        fs_write(&root_file, root_data, strlen(root_data));
        fs_close(&root_file);
    }
    
    // 配置文件
    fs_file_t config_file;
    if (fs_open(&config_file, PATH_CAT("config/settings.cfg"), FS_O_CREATE | FS_O_WRITE) == 0) {
        const char* config_data = "[system]\nversion=1.0\n";
        fs_write(&config_file, config_data, strlen(config_data));
        fs_close(&config_file);
    }
    
    // 日志文件
    fs_file_t log_file;
    if (fs_open(&log_file, PATH_CAT("logs/system.log"), FS_O_CREATE | FS_O_WRITE) == 0) {
        const char* log_data = "2023-07-31 10:00:00 System started\n";
        fs_write(&log_file, log_data, strlen(log_data));
        fs_close(&log_file);
    }
    
    // 二进制文件
    fs_file_t bin_file;
    if (fs_open(&bin_file, PATH_CAT("data/images/thumb.dat"), FS_O_CREATE | FS_O_WRITE) == 0) {
        uint8_t bin_data[4] = {0x89, 0x50, 0x4E, 0x47}; // PNG头
        fs_write(&bin_file, bin_data, sizeof(bin_data));
        fs_close(&bin_file);
    }
#endif
    //列出所有目录内容
    app_log_info("||||||||||||||||||||||||||||||||||||||||\r\n");
    ret = lsdir(MOUNT_POINT_CAT);
    if (ret < 0) {
        app_log_error("FAIL: lsdir %s: %d\r\n", MOUNT_POINT_CAT, ret);
    }
    app_log_info("||||||||||||||||||||||||||||||||||||||||\r\n");

#if FILESYSTEM_CREATE_TEST_FILES
    app_log_info("========================================\r\n");
    app_log_info("Listing config directory:\r\n");
    ret = lsdir(PATH_CAT("config"));
    if (ret < 0) {
        app_log_error("FAIL: lsdir config: %d\r\n", ret);
    }
    
    app_log_info("========================================\r\n");
    app_log_info("Listing data/images directory:\r\n");
    ret = lsdir(PATH_CAT("data/images"));
    if (ret < 0) {
        app_log_error("FAIL: lsdir data/images: %d\r\n", ret);
    }
    app_log_info("========================================\r\n");
#endif

    // // 创建挂载点目录
    // if (fs_mkdir(fs_mount_cat.mnt_point) != 0) {
    //     app_log_error("Failed to create mount point directory\r\n");
    //     return;
    // }


    // app_log_debug("fs_open success\r\n");
    // // 写入数据
    // const char* data = "Hello CatCollar!";
    // ssize_t written = fs_write(&file, data, strlen(data));
    // if (written < 0) {
    //     app_log_error("Write failed: %d\r\n", (int)written);
    // }
    // app_log_debug("fs_write success\r\n");
    // // 同步数据
    // fs_sync(&file);
    
    // // 关闭文件
    // fs_close(&file);
    // app_log_debug("fs_close success\r\n");
    
    // // 卸载文件系统
    // fs_unmount(&fs_mount_cat);
    // app_log_debug("fs_unmount success\r\n");

    return 0;
}

file_handle_t storage_open_file(char *fname)
{
	if (m_open_file_count >= MAX_OPEN_FILES) {
		app_log_error("max open file count exceeded: %d\r\n", m_open_file_count);
		return NULL;
	}

	struct file_data_t *file_data = &m_file_data[m_open_file_count];

	// imu file
	snprintf(file_data->imu_filename, MAX_PATH_LEN, "%s/%s.imu", fs_mount_cat.mnt_point, fname);

	file_data->imu_sample_count = 0;

	fs_file_t_init(&file_data->imu_file);
	int ret = fs_open(&file_data->imu_file, file_data->imu_filename, FS_O_CREATE | FS_O_RDWR);
	if (ret < 0) {
		app_log_error("FAIL: open %s: %d\r\n", file_data->imu_filename, ret);
		return NULL;
	}

	// // activity file
	// snprintf(file_data->activity_filename, MAX_PATH_LEN, "%s/%s.act", mp->mnt_point, fname);
	// file_data->activity_sample_count = 0;
	// fs_file_t_init(&file_data->activity_file);
	// ret = fs_open(&file_data->activity_file, file_data->activity_filename,
	// 	     FS_O_CREATE | FS_O_RDWR);
	// if (ret < 0) {
	// 	app_log_error("FAIL: open %s: %d", file_data->activity_filename, ret);
	// 	return NULL;
	// }

	// // increment the file_id count 
	// rec_ses_count++;

	m_open_file_count++;
	return file_data;
}
int storage_close_file(file_handle_t handle)
{
    // 增加强制同步
    int sync_ret = fs_sync(&handle->imu_file);
    app_log_info("FORCE SYNC before close: %d", sync_ret);
    int ret = fs_close(&handle->imu_file);
	if (ret < 0) {
        app_log_error("FAIL: close %s: %d\r\n", handle->imu_filename, ret);
		return ret;
	}
    
	// ret = fs_close(&handle->activity_file);

	// if (ret < 0) {
	// 	app_log_error("FAIL: close %s: %d", handle->activity_filename, ret);
	// 	return ret;
	// }

	m_open_file_count--;
	return 0;
}

int storage_write_raw_imu_record(file_handle_t handle, raw_imu_record_t raw_record)
{
	int ret = fs_write(&handle->imu_file, &raw_record, sizeof(raw_record));
	if (ret < 0) {
		app_log_error("FAIL: write %s: %d\r\n", handle->imu_filename, ret);
		erase_and_reboot();
		return -1;
	}
	return 0;
}

// static int storage_print(const struct shell *sh, size_t argc, char **argv)
// {
// 	fs_file_t file;
// 	uint16_t sample_count = 0;
// 	char *fname = argv[1];

// 	fs_file_t_init(&file);
// 	int rc = fs_open(&file, fname, FS_O_RDWR);
// 	if (rc < 0) {
// 		app_log_error("FAIL: open %s: %d", fname, rc);
// 		return rc;
// 	}

// 	rc = fs_seek(&file, 0, FS_SEEK_SET);
// 	if (rc < 0) {
// 		app_log_error("FAIL: seek %s: %d", fname, rc);
// 		return rc;
// 	}

// 	for (;;) {
// 		raw_imu_record_t record;

// 		rc = fs_read(&file, &record, sizeof(record));
// 		if (rc == 0) {
// 			app_log_info("complete");
// 			break;
// 		}

// 		else if (rc < 0 || rc < sizeof(record)) {
// 			app_log_error("FAIL: read %s: [rd:%d]", fname, rc);
// 			break;
// 		}

// 		app_log_info("%d:\t record_num=%d, time=%llu", sample_count++,
// 			sys_be32_to_cpu(record.record_num), sys_be64_to_cpu(record.timestamp));
// 		for (int i = 0; i < RAW_FRAMES_PER_RECORD; i++) {
// 			app_log_info("ax=%0.3f, ay=%0.3f, az=%0.3f", record.raw_data[i].ax,
// 				record.raw_data[i].ay, record.raw_data[i].az);
// 			app_log_info("gx=%0.3f, gy=%0.3f, gz=%0.3f", record.raw_data[i].gx,
// 				record.raw_data[i].gy, record.raw_data[i].gz);
// 		}
// 		LOG_PANIC(); // flush
// 	}

// 	rc = fs_close(&file);
// 	if (rc < 0) {
// 		app_log_error("FAIL: close %s: %d", fname, rc);
// 		return rc;
// 	}

// 	return 0;
// }

//static int storage_ls(const struct shell *sh, size_t argc, char **argv)
//{
//	int ret = lsdir(fs_mount_cat.mnt_point);
//	if (ret < 0) {
//		app_log_error("FAIL: lsdir %s: %d\r\n", fs_mount_cat.mnt_point, ret);
//		return ret;
//	}
//
//	return 0;
//}

static int lsdir(const char *path)
{
    fs_dir_t dirp;
    fs_dirent entry;
    int ret;

    fs_dir_t_init(&dirp);

    ret = fs_opendir(&dirp, path);
    if (ret) {
        app_log_error("Error opening dir %s [%d]\r\n", path, ret);
        return ret;
    }

    while (1) 
    {
        ret = fs_readdir(&dirp, &entry);
        if (ret <= 0 || entry.name[0] == '\0') {
            // app_log_debug("entry.name = %s\r\n", entry.name);
            // app_log_debug("entry.type = %d\r\n", entry.type);
            // app_log_debug("entry.size = %lu\r\n", entry.size);
            break;
        }

        // 忽略当前目录和上级目录
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
            continue;
        }
        
        if (entry.type == FS_DIR_ENTRY_DIR) {
            app_log_info("[DIR ] %s\r\n", entry.name);
        } else {
            app_log_info("[FILE] %s (size = %lu)\r\n", entry.name, entry.size);
        }
    }

    // 检查读取错误
    if (ret < 0) {
        app_log_error("Error reading dir [%d]\r\n", ret);
    }

    fs_closedir(&dirp);
    return ret;
}

// returns the number of records in 'filename'
int storage_get_raw_imu_record_count(char *basename)
{
	fs_dirent dirent;
	char fname[MAX_PATH_LEN];

	snprintf(fname, sizeof(fname), "%s/%s.imu", fs_mount_cat.mnt_point, basename);

	int rc = fs_stat(fname, &dirent);

	if (rc >= 0) {
		app_log_debug("\tfn '%s' siz %u\r\n", dirent.name, dirent.size);
	} else {
		app_log_error("cannot stat %s\r\n", basename);
		return -1;
	}

	int records = dirent.size / sizeof(raw_imu_record_t);

	app_log_debug("%s records: %d\r\n", fname, records);
	return records;
}


int storage_read_raw_imu_record(char *basename, int record_number, raw_imu_record_t *record)
{
	fs_file_t file;
	char fname[MAX_PATH_LEN];
	
	int total_records = storage_get_raw_imu_record_count(basename);
	if (record_number < total_records) {

	} else {
		return -1;
	}

	snprintf(fname, sizeof(fname), "%s/%s.imu", fs_mount_cat.mnt_point, basename);

	fs_file_t_init(&file);
	//int rc = fs_open(&file, fname, FS_O_RDWR);
	int rc = fs_open(&file, fname, FS_O_READ);				// FS_O_READ instead of FS_O_RDWR, changed on 2021-09-29, for CHEKR_CONT_REC
	if (rc < 0) {
		app_log_error("FAIL: open %s: %d\r\n", fname, rc);
		return rc;
	}

	int seek_offset = record_number * sizeof(raw_imu_record_t);

	rc = fs_seek(&file, seek_offset, FS_SEEK_SET);
	if (rc < 0) {
		app_log_error("FAIL: seek %s: %d\r\n", fname, rc);
		return rc;
	}

	rc = fs_read(&file, record, sizeof(*record));
	if (rc == 0) {
	}

	else if (rc < 0 || (unsigned int)rc < sizeof(*record)) {
		app_log_error("FAIL: read %s: [rd:%d]\r\n", fname, rc);
		return -1;
	}

	rc = fs_close(&file);
	if (rc < 0) {
		app_log_error("FAIL: close %s: %d\r\n", fname, rc);
		return -1;
	}

#define ERASE_FILE_AFTER_HARVESTING (1)
#ifdef ERASE_FILE_AFTER_HARVESTING
	
	if (record_number == total_records - 1) {
		app_log_info("read last record (%d), deleting %s!\r\n", record_number, fname);
		// rc = fs_unlink(fname);
		// if (rc < 0) {
		// 	app_log_error("error unlinking %s", fname);
		// 	return rc;
		// }
	}
#endif

	return 0;
}

int storage_delete_activity_file(char *basename)
{
	char fname[128];
	snprintf(fname, sizeof(fname), "%s/%s.act", fs_mount_cat.mnt_point, basename);
	int ret = fs_unlink(fname);
	if (ret < 0) {
		app_log_error("error unlinking %s\r\n", fname);
		return ret;
	}
	return 0;
}

/************************************
 * Activity related interfaces
 ************************************/
int storage_write_activity_record(file_handle_t handle, activity_record_t raw_record)
{
	int rc = fs_write(&handle->activity_file, &raw_record, sizeof(raw_record));
	if (rc < 0) {
		app_log_error("FAIL: write %s: %d\r\n", handle->imu_filename, rc);
		erase_and_reboot();
		return -1;
	}
	return 0;
}

int storage_get_activity_record_count(char *basename)
{
	fs_dirent dirent;
	char fname[MAX_PATH_LEN];

	snprintf(fname, sizeof(fname), "%s/%s.act", fs_mount_cat.mnt_point, basename);

	int rc = fs_stat(fname, &dirent);

	if (rc >= 0) {
		app_log_debug("\tfn '%s' siz %u\r\n", dirent.name, dirent.size);
	} else {
		app_log_error("cannot stat %s\r\n", basename);
		return -1;
	}

	int records = dirent.size / sizeof(activity_record_t);

	app_log_debug("%s records: %d\r\n", fname, records);
	return records;
}

int storage_read_activity_record(char *basename, int record_number, activity_record_t *record)
{
	fs_file_t file;
	char fname[MAX_PATH_LEN];

	snprintf(fname, sizeof(fname), "%s/%s.act", fs_mount_cat.mnt_point, basename);

	fs_file_t_init(&file);
	int rc = fs_open(&file, fname, FS_O_RDWR);
	if (rc < 0) {
		app_log_error("FAIL: open %s: %d\r\n", fname, rc);
		return rc;
	}

	int seek_offset = record_number * sizeof(activity_record_t);

	rc = fs_seek(&file, seek_offset, FS_SEEK_SET);
	if (rc < 0) {
		app_log_error("FAIL: seek %s: %d\r\n", fname, rc);
		return rc;
	}

	rc = fs_read(&file, record, sizeof(*record));
	if (rc == 0) {
	}

	else if (rc < 0 || (unsigned int)rc < sizeof(*record)) {
		app_log_error("FAIL: read %s: [rd:%d]\r\n", fname, rc);
		return -1;
	}

	rc = fs_close(&file);
	if (rc < 0) {
		app_log_error("FAIL: close %s: %d\r\n", fname, rc);
		return -1;
	}

#define ERASE_FILE_AFTER_HARVESTING (1)
#ifdef ERASE_FILE_AFTER_HARVESTING
	int total_records = storage_get_activity_record_count(basename);
	if (record_number == total_records - 1) {
		app_log_info("read last record (%d), deleting %s!\r\n", record_number, fname);
		// rc = fs_unlink(fname);
		// if (rc < 0) {
		// 	app_log_error("error unlinking %s", fname);
		// 	return rc;
		// }
	}
#endif

	return 0;
}

// if we hit flash errors, it's likely because we ran out of disk space
// in this case, erase the flash and reboot to put the device back into a usable state
static void erase_and_reboot(void)
{
	// app_log_error("erasing flash!");
	// int rc = erase_flash((uintptr_t)mp->storage_dev);
	// if (rc < 0) {
	// 	app_log_error("failure: erase_flash rc=%d", rc);
	// }

	app_log_error("rebooting device!\r\n");
	// sys_reboot(SYS_REBOOT_COLD);
}


// // returns the number of records in 'filename'
// int storage_get_raw_imu_record_count(char *basename)
// {
//     fs_status stat;
// 	char fname[MAX_PATH_LEN];

// 	snprintf(fname, sizeof(fname), "%s/%s.imu", fs_mount_cat.mnt_point, basename);

// 	int ret = fs_stat(fname, &stat);

// 	if (ret >= 0) {
// 		app_log_debug("\tfile: '%s' size: %u\r\n", stat.name, stat.size);
// 	} else {
// 		app_log_error("cannot stat %s\r\n", basename);
// 		return -1;
// 	}

// 	int records = stat.size / sizeof(raw_imu_record_t);

// 	app_log_debug("[%s] records: %d\r\n", fname, records);
// 	return records;
// }

// Function to erase a continuous recording chunk.
int storage_delete_cont_rec_chunk_file(char *basename)
{
	char fname_imu[128];
	char fname_act[128];

	snprintf(fname_imu, sizeof(fname_imu), "%s/%s.imu", fs_mount_cat.mnt_point, basename);
	int ret1 = fs_unlink(fname_imu);
	if (ret1 < 0) {
		app_log_error("error unlinking %s\r\n", fname_imu);
		return ret1;
	}

	snprintf(fname_act, sizeof(fname_act), "%s/%s.act", fs_mount_cat.mnt_point, basename);
	int ret2 = fs_unlink(fname_act);
	if (ret2 < 0) {
		app_log_error("error unlinking %s\r\n", fname_act);
		return ret2;
	}
	return 0;
}

// Function to erase flash.
int delete_all_files()
{
	int ret = delete_all_files_in_mount_point(fs_mount_cat.mnt_point);
	if (ret < 0) {
		app_log_error("failure: erase_flash ret=%d\r\n", ret);
		return ret;
	}

	return 0;
}
// Function to erase all files in a mount point.
static int delete_all_files_in_mount_point(const char *mount_point)
{
    fs_dir_t dir;
    fs_dirent entry;
    int ret;

    fs_dir_t_init(&dir);

    ret = fs_opendir(&dir, mount_point);
    if (ret < 0) {
        app_log_error("Failed to open directory %s: %d\r\n", mount_point, ret);
        return ret;
    }

    while (1) 
    {
        ret = fs_readdir(&dir, &entry);
        if (ret < 0) {
            app_log_error("Failed to read directory %s: %d\r\n", mount_point, ret);
            fs_closedir(&dir);
            return ret;
        }

        if (ret == 0 || entry.name[0] == '\0') {
            // End of directory
            break;
        }

        if (entry.type == FS_DIR_ENTRY_FILE) {
            char filepath[512];
			snprintf(filepath, sizeof(filepath), "%s/%s", mount_point, entry.name);
			//sprintf(filepath, "%s/%s", mount_point, entry.name);
            ret = fs_unlink(filepath);
            if (ret < 0) {
                app_log_error("Failed to delete file %s: %d\r\n", filepath, ret);
                fs_closedir(&dir);
                return ret;
            }
        }
    }

    fs_closedir(&dir);
    return 0;
}

