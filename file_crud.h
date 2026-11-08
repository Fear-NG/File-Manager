#ifndef FILE_CRUD_H
#define FILE_CRUD_H

#include "file_types.h"

// Thao tác CRUD tệp & thư mục
int file_copy(const char *src, const char *dst);
int file_move(const char *src, const char *dst);
int file_delete_recursive(const char *path);
int file_mkdir(const char *path);
int file_touch(const char *path);

// Hàm định dạng hiển thị
void file_format_size(off_t bytes, char *out, size_t out_sz);
void file_format_time(time_t mtime, char *out, size_t out_sz);
void file_format_perms(mode_t mode, char *out, size_t out_sz);

// Lấy thông tin dung lượng ổ đĩa
int get_disk_space(const char *path, char *free_out, size_t free_sz, char *total_out, size_t total_sz);

#endif // FILE_CRUD_H

