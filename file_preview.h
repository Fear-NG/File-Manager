#ifndef FILE_PREVIEW_H
#define FILE_PREVIEW_H

#include "file_types.h"

// Kiểm tra phần mở rộng tệp (không phân biệt hoa/thường)
int has_extension(const char *filepath, const char *ext);

// Trích xuất nội dung xem trước đa định dạng (Text, PDF, DOCX, Thư mục, Hex binary)
void get_file_preview(const char *filepath, char lines[][PREVIEW_LINE_LEN], int max_lines, int *actual_lines);

#endif // FILE_PREVIEW_H

