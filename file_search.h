#ifndef FILE_SEARCH_H
#define FILE_SEARCH_H

#include "file_types.h"

// Thu thập cấu trúc cây thư mục (dạng nhánh cây ├── └──)
void get_dir_tree(const char *dir_path, char lines[][PREVIEW_LINE_LEN], int max_lines, int *actual_lines, int max_depth);

// Tìm kiếm tệp & thư mục đệ quy theo từ khóa
void search_files_recursive(const char *base_dir, const char *keyword, SearchResult *results, int max_results, int *actual_results);

#endif // FILE_SEARCH_H

