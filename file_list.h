#ifndef FILE_LIST_H
#define FILE_LIST_H

#include "file_types.h"

// Khởi tạo và giải phóng danh sách tệp
void file_list_init(FileList *list);
void file_list_free(FileList *list);

// Tải danh sách tệp từ thư mục với bộ lọc, tùy chọn ẩn/hiện tệp ẩn và chế độ sắp xếp
int file_list_load(FileList *list, const char *dir_path, const char *filter, int show_hidden, SortMode sort_mode);

// Tên hiển thị của chế độ sắp xếp
const char* sort_mode_name(SortMode mode);

#endif // FILE_LIST_H

