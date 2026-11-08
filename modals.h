#ifndef MODALS_H
#define MODALS_H

#include "file_types.h"

// Bảng trợ giúp phím tắt điều khiển
void show_help_modal(void);

// Bảng thông tin chi tiết / siêu dữ liệu tệp hoặc thư mục
void show_info_modal(const FileItem *item);

// Cửa sổ sơ đồ cây thư mục (Tree View) có cuộn
void show_tree_modal(const char *current_dir);

// Cửa sổ tìm kiếm đệ quy toàn diện với khả năng chọn nhảy đến tệp
int show_search_modal(const char *current_dir, char *chosen_path, int *chosen_is_dir);

// Trình đọc tệp văn bản / mã nguồn toàn màn hình (File Pager)
void show_pager_modal(const char *filepath);

#endif // MODALS_H
