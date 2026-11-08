#ifndef FILE_TYPES_H
#define FILE_TYPES_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LEN 1024
#define PREVIEW_MAX_LINES 60
#define PREVIEW_LINE_LEN 512

// Chế độ sắp xếp tệp
typedef enum {
    SORT_NAME_ASC = 0,   // Tên A -> Z
    SORT_NAME_DESC,      // Tên Z -> A
    SORT_SIZE_DESC,      // Dung lượng giảm dần
    SORT_SIZE_ASC,       // Dung lượng tăng dần
    SORT_TIME_DESC,      // Sửa đổi mới nhất
    SORT_TIME_ASC        // Sửa đổi cũ nhất
} SortMode;

// Thao tác với bộ nhớ đệm (Clipboard)
typedef enum {
    CLIP_NONE = 0,
    CLIP_COPY,
    CLIP_CUT
} ClipboardOp;

// Thông tin của một tệp hoặc thư mục
typedef struct {
    char name[256];
    char full_path[MAX_PATH_LEN];
    mode_t mode;
    off_t size;
    time_t mtime;
    int is_dir;
    int is_symlink;
    int is_exec;
} FileItem;

// Danh sách động chứa các tệp
typedef struct {
    FileItem *items;
    int count;
    int capacity;
} FileList;

// Kết quả tìm kiếm tệp
typedef struct {
    char path[MAX_PATH_LEN];
    char name[256];
    int is_dir;
    off_t size;
} SearchResult;

// Trạng thái toàn cục của ứng dụng
typedef struct {
    char current_dir[MAX_PATH_LEN];
    FileList file_list;
    int selected_index;
    int scroll_offset;
    char filter[128];
    int show_hidden;
    SortMode sort_mode;

    // Bộ nhớ đệm (Clipboard)
    char clip_path[MAX_PATH_LEN];
    char clip_name[256];
    ClipboardOp clip_op;

    // Thanh thông báo trạng thái
    char status_msg[256];
    int status_type; // 0 = info (vàng), 1 = success (xanh lá), 2 = error (đỏ)
    time_t status_expire;

    // Bộ đệm xem trước (Preview Cache)
    int cached_preview_index;
    char cached_path[MAX_PATH_LEN];
    char preview_lines[PREVIEW_MAX_LINES][PREVIEW_LINE_LEN];
    int preview_line_count;
} AppState;

// Hàm sao chép chuỗi an toàn chống tràn bộ đệm
static inline void safe_copy_str(char *dst, const char *src, size_t dst_size) {
    if (dst_size == 0) return;
    size_t i = 0;
    while (i + 1 < dst_size && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

#endif // FILE_TYPES_H

