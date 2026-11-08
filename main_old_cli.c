#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH 1024
#define MAX_ITEMS 100
#define PREVIEW_LINES 15

// Biến toàn cục lưu file được chọn mặc định
char g_selected_file[MAX_PATH] = "";

void clean_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';
}

int has_extension(const char *filepath, const char *ext) {
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return 0;
    return strcasecmp(dot, ext) == 0;
}

void run_cmd_preview(const char *cmd, int max_lines) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return;
    char buf[512];
    int count = 0;
    while (fgets(buf, sizeof(buf), fp)) {
        printf("%s", buf);
        if (max_lines > 0 && ++count >= max_lines) {
            printf("... [Đã thu gọn nội dung] ...\n");
            break;
        }
    }
    pclose(fp);
}

// Bảng chọn file
int pick_file(const char *target_dir, char *selected_path) {
    DIR *dir = opendir(target_dir);
    if (!dir) {
        perror("Không thể mở thư mục để liệt kê file");
        return 0;
    }

    struct dirent *entry;
    char file_list[MAX_ITEMS][MAX_PATH];
    int count = 0;

    printf("\nDanh sách file trong '%s':\n", target_dir);
    printf(" --------------------------------------------------\n");

    while ((entry = readdir(dir)) != NULL && count < MAX_ITEMS) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[MAX_PATH * 2];
        snprintf(full, sizeof(full), "%s/%s", target_dir, entry->d_name);
        struct stat st;
        if (stat(full, &st) == 0 && S_ISREG(st.st_mode)) {
            strncpy(file_list[count], full, MAX_PATH - 1);
            file_list[count][MAX_PATH - 1] = '\0';
            printf("  [%d] %-30s (%.2f KB)\n", count + 1, entry->d_name, (double)st.st_size / 1024);
            count++;
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("  (Không tìm thấy file nào trong thư mục này)\n");
    }

    printf("  [0] Tự nhập đường dẫn khác bằng tay\n");
    printf(" --------------------------------------------------\n");

    char buf[64];
    while (1) {
        printf("Chọn file theo số [0-%d]: ", count);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        clean_newline(buf);
        if (strlen(buf) == 0) continue;

        char *endptr;
        long choice = strtol(buf, &endptr, 10);
        if (*endptr != '\0' || choice < 0 || choice > count) {
            printf(">> Số chọn không hợp lệ! Vui lòng nhập lại.\n");
            continue;
        }

        if (choice == 0) {
            printf("Nhập đường dẫn file: ");
            if (!fgets(selected_path, MAX_PATH, stdin)) return 0;
            clean_newline(selected_path);
            return (strlen(selected_path) > 0);
        } else {
            strncpy(selected_path, file_list[choice - 1], MAX_PATH - 1);
            selected_path[MAX_PATH - 1] = '\0';
            return 1;
        }
    }
}

// Bảng chọn thư mục đích
int pick_directory(const char *base_dir, char *selected_dir) {
    DIR *dir = opendir(base_dir);
    if (!dir) {
        perror("Không thể mở thư mục để liệt kê");
        return 0;
    }

    struct dirent *entry;
    char dir_list[MAX_ITEMS][MAX_PATH];
    int count = 0;

    strncpy(dir_list[count], ".", MAX_PATH - 1);
    dir_list[count][MAX_PATH - 1] = '\0';
    printf("\nDanh sách thư mục đích khả dụng:\n");
    printf(" --------------------------------------------------\n");
    printf("  [1] . (Thư mục gốc Midterm hiện tại)\n");
    count++;

    while ((entry = readdir(dir)) != NULL && count < MAX_ITEMS) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[MAX_PATH * 2];
        snprintf(full, sizeof(full), "%s/%s", base_dir, entry->d_name);
        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            strncpy(dir_list[count], full, MAX_PATH - 1);
            dir_list[count][MAX_PATH - 1] = '\0';
            printf("  [%d] %s/\n", count + 1, entry->d_name);
            count++;
        }
    }
    closedir(dir);

    printf("  [0] Tự nhập đường dẫn thư mục đích bằng tay\n");
    printf(" --------------------------------------------------\n");

    char buf[64];
    while (1) {
        printf("Chọn thư mục đích theo số [0-%d]: ", count);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        clean_newline(buf);
        if (strlen(buf) == 0) continue;

        char *endptr;
        long choice = strtol(buf, &endptr, 10);
        if (*endptr != '\0' || choice < 0 || choice > count) {
            printf(">> Số chọn không hợp lệ! Vui lòng nhập lại.\n");
            continue;
        }

        if (choice == 0) {
            printf("Nhập đường dẫn đích: ");
            if (!fgets(selected_dir, MAX_PATH, stdin)) return 0;
            clean_newline(selected_dir);
            return (strlen(selected_dir) > 0);
        } else {
            strncpy(selected_dir, dir_list[choice - 1], MAX_PATH - 1);
            selected_dir[MAX_PATH - 1] = '\0';
            return 1;
        }
    }
}

// 1. Quản lý chọn / đổi file mặc định
void select_or_change_file() {
    if (strlen(g_selected_file) > 0) {
        printf("\nFile đang chọn hiện tại là: '%s'\n", g_selected_file);
        printf("  [1] Chọn file khác\n");
        printf("  [2] Hủy chọn (Không dùng file mặc định)\n");
        printf("  [0] Quay lại menu\n");
        printf("Chọn thao tác: ");
        char buf[16];
        if (!fgets(buf, sizeof(buf), stdin)) return;
        clean_newline(buf);
        if (strcmp(buf, "2") == 0) {
            g_selected_file[0] = '\0';
            printf(">> Đã hủy chọn file mặc định.\n");
            return;
        } else if (strcmp(buf, "1") != 0) {
            return;
        }
    }

    char temp_path[MAX_PATH];
    if (pick_file("test_files", temp_path) || pick_file(".", temp_path)) {
        strncpy(g_selected_file, temp_path, sizeof(g_selected_file) - 1);
        g_selected_file[sizeof(g_selected_file) - 1] = '\0';
        printf(">> Đã đặt '%s' làm file mặc định thành công!\n", g_selected_file);
    }
}

// 2. Xem thông tin file
void show_file_info() {
    char filepath[MAX_PATH];
    if (strlen(g_selected_file) > 0) {
        strncpy(filepath, g_selected_file, sizeof(filepath) - 1);
        filepath[sizeof(filepath) - 1] = '\0';
        printf("\n>> Đang dùng file mặc định: %s\n", filepath);
    } else {
        if (!pick_file("test_files", filepath) && !pick_file(".", filepath)) return;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        perror("Lỗi đọc thông tin file");
        return;
    }

    printf("\n=== THÔNG TIN HỆ THỐNG ===\n");
    printf("Đường dẫn  : %s\n", filepath);
    printf("Dung lượng : %ld bytes (%.2f KB)\n", (long)st.st_size, (double)st.st_size / 1024);

    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    printf("Sửa đổi    : %s\n", time_buf);

    if (!S_ISREG(st.st_mode)) return;

    if (has_extension(filepath, ".pdf")) {
        char cmd[MAX_PATH * 2];
        printf("\n--- [Metadata PDF] ---\n");
        snprintf(cmd, sizeof(cmd), "pdfinfo \"%s\" 2>/dev/null", filepath);
        run_cmd_preview(cmd, 15);

        printf("\n--- [Nội dung PDF trích xuất] ---\n");
        snprintf(cmd, sizeof(cmd), "pdftotext \"%s\" - 2>/dev/null", filepath);
        run_cmd_preview(cmd, PREVIEW_LINES);
    } else if (has_extension(filepath, ".docx")) {
        char cmd[MAX_PATH * 2];
        printf("\n--- [Metadata DOCX] ---\n");
        snprintf(cmd, sizeof(cmd), "exiftool -Title -Creator -CreateDate -WordCount \"%s\" 2>/dev/null", filepath);
        run_cmd_preview(cmd, 10);

        printf("\n--- [Nội dung DOCX trích xuất] ---\n");
        snprintf(cmd, sizeof(cmd), "docx2txt < \"%s\" 2>/dev/null", filepath);
        run_cmd_preview(cmd, PREVIEW_LINES);
    } else {
        printf("\n--- [Xem trước nội dung (%d dòng đầu)] ---\n", PREVIEW_LINES);
        FILE *fp = fopen(filepath, "r");
        if (!fp) return;
        char line[512];
        int count = 0;
        while (fgets(line, sizeof(line), fp) && count++ < PREVIEW_LINES) {
            printf("%s", line);
        }
        fclose(fp);
    }
    printf("\n==========================\n");
}

// 3. Hiển thị cây thư mục
void print_tree(const char *base, int depth) {
    DIR *dir = opendir(base);
    if (!dir) return;
    struct dirent *entry;
    char path[MAX_PATH * 2];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        for (int i = 0; i < depth; i++) printf("│   ");
        printf("├── %s\n", entry->d_name);

        snprintf(path, sizeof(path), "%s/%s", base, entry->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode) && depth < 4) {
            print_tree(path, depth + 1);
        }
    }
    closedir(dir);
}

void show_directory_tree() {
    char path[MAX_PATH];
    printf("\nNhập thư mục (Enter để xem thư mục Midterm hiện tại): ");
    fgets(path, sizeof(path), stdin);
    clean_newline(path);
    if (strlen(path) == 0) strcpy(path, ".");
    printf("\n%s\n", path);
    print_tree(path, 0);
}

// 4. Di chuyển / Đổi tên file
void move_file() {
    char src[MAX_PATH], dest_dir[MAX_PATH];
    char dest_path[MAX_PATH * 3];

    if (strlen(g_selected_file) > 0) {
        strncpy(src, g_selected_file, sizeof(src) - 1);
        src[sizeof(src) - 1] = '\0';
        printf("\n>> Đang thao tác trên file nguồn mặc định: '%s'\n", src);
    } else {
        printf("\n--- Chọn file nguồn cần di chuyển ---");
        if (!pick_file("test_files", src) && !pick_file(".", src)) return;
    }

    if (!pick_directory(".", dest_dir)) return;

    const char *filename = strrchr(src, '/');
    if (filename) filename++;
    else filename = src;

    char new_name[256];
    printf("Tên file ở vị trí đích (Enter để giữ nguyên '%s'): ", filename);
    fgets(new_name, sizeof(new_name), stdin);
    clean_newline(new_name);

    if (strlen(new_name) == 0) {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, filename);
    } else {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, new_name);
    }

    if (rename(src, dest_path) == 0) {
        printf(">> Đã di chuyển '%s' -> '%s' thành công!\n", src, dest_path);
        if (strcmp(src, g_selected_file) == 0) {
            strncpy(g_selected_file, dest_path, sizeof(g_selected_file) - 1);
            g_selected_file[sizeof(g_selected_file) - 1] = '\0';
        }
    } else {
        perror("Lỗi di chuyển");
    }
}

// 5. Sao chép file
void copy_file() {
    char src[MAX_PATH], dest_dir[MAX_PATH];
    char dest_path[MAX_PATH * 3];

    if (strlen(g_selected_file) > 0) {
        strncpy(src, g_selected_file, sizeof(src) - 1);
        src[sizeof(src) - 1] = '\0';
        printf("\n>> Đang thao tác trên file nguồn mặc định: '%s'\n", src);
    } else {
        printf("\n--- Chọn file nguồn cần sao chép ---");
        if (!pick_file("test_files", src) && !pick_file(".", src)) return;
    }

    if (!pick_directory(".", dest_dir)) return;

    const char *filename = strrchr(src, '/');
    if (filename) filename++;
    else filename = src;

    char new_name[256];
    printf("Đặt tên cho file sao chép (Enter để thêm tiền tố 'copy_'): ");
    fgets(new_name, sizeof(new_name), stdin);
    clean_newline(new_name);

    if (strlen(new_name) == 0) {
        snprintf(dest_path, sizeof(dest_path), "%s/copy_%s", dest_dir, filename);
    } else {
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, new_name);
    }

    FILE *fs = fopen(src, "rb"), *fd = fopen(dest_path, "wb");
    if (!fs || !fd) {
        perror("Lỗi mở file để sao chép");
        if (fs) fclose(fs);
        return;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fs)) > 0) fwrite(buf, 1, n, fd);
    fclose(fs); fclose(fd);
    printf(">> Đã sao chép thành công tới '%s'!\n", dest_path);
}

// 6. Xóa file
void delete_file() {
    char target[MAX_PATH];

    if (strlen(g_selected_file) > 0) {
        strncpy(target, g_selected_file, sizeof(target) - 1);
        target[sizeof(target) - 1] = '\0';
        printf("\n>> Đang thao tác trên file mặc định: '%s'\n", target);
    } else {
        printf("\n--- Chọn file cần xóa ---");
        if (!pick_file("test_files", target) && !pick_file(".", target)) return;
    }

    char confirm[10];
    printf("Bạn có chắc chắn muốn xóa '%s'? (y/n): ", target);
    fgets(confirm, sizeof(confirm), stdin);
    if (confirm[0] == 'y' || confirm[0] == 'Y') {
        if (unlink(target) == 0) {
            printf(">> Đã xóa file '%s' thành công!\n", target);
            if (strcmp(target, g_selected_file) == 0) g_selected_file[0] = '\0';
        } else {
            perror("Lỗi xóa file");
        }
    } else {
        printf(">> Đã hủy thao tác xóa.\n");
    }
}

// 7. Tìm kiếm file
void search_files_recursive(const char *base_dir, const char *keyword, int *match_count) {
    DIR *dir = opendir(base_dir);
    if (!dir) return;

    struct dirent *entry;
    char full_path[MAX_PATH * 2];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, entry->d_name);
        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (strcasestr(entry->d_name, keyword) != NULL) {
            (*match_count)++;
            if (S_ISDIR(st.st_mode)) {
                printf(" [%d] [DIR ] %s\n", *match_count, full_path);
            } else {
                printf(" [%d] [FILE] %s (%.2f KB)\n", *match_count, full_path, (double)st.st_size / 1024);
            }
        }

        if (S_ISDIR(st.st_mode)) {
            search_files_recursive(full_path, keyword, match_count);
        }
    }
    closedir(dir);
}

void search_file() {
    char keyword[128];
    char start_dir[MAX_PATH];

    printf("\nNhập từ khóa hoặc đuôi mở rộng (VD: demo, .txt, .pdf): ");
    fgets(keyword, sizeof(keyword), stdin); clean_newline(keyword);

    if (strlen(keyword) == 0) {
        printf(">> Từ khóa không được để trống!\n");
        return;
    }

    printf("Nhập thư mục tìm kiếm (Enter để tìm từ thư mục hiện tại '.'): ");
    fgets(start_dir, sizeof(start_dir), stdin); clean_newline(start_dir);
    if (strlen(start_dir) == 0) strcpy(start_dir, ".");

    printf("\n--- Kết quả tìm kiếm cho '%s' trong '%s' ---\n", keyword, start_dir);
    int match_count = 0;
    search_files_recursive(start_dir, keyword, &match_count);

    if (match_count == 0) {
        printf(">> Không tìm thấy kết quả nào.\n");
    } else {
        printf("--------------------------------------------\n");
        printf(">> Tìm thấy: %d mục.\n", match_count);
    }
}

int main() {
    char buf[64];
    char *endptr;
    long choice;

    while (1) {
        printf("\n================ MIDTERM FILE MANAGER ================\n");
        if (strlen(g_selected_file) > 0) {
            printf(" [File đang chọn: %s]\n", g_selected_file);
        } else {
            printf(" [Chưa chọn file mặc định]\n");
        }
        printf("------------------------------------------------------\n");
        printf(" 1. Chọn / Đổi file mặc định\n");
        printf(" 2. Xem thông tin & nội dung file (TXT/PDF/DOCX)\n");
        printf(" 3. Hiển thị cây thư mục\n");
        printf(" 4. Di chuyển / Đổi tên file (Cho chọn thư mục đích)\n");
        printf(" 5. Sao chép file\n");
        printf(" 6. Xóa file\n");
        printf(" 7. Tìm kiếm file theo tên hoặc đuôi (.txt, .pdf,...)\n");
        printf(" 0. Thoát\n");
        printf("======================================================\n");

        while (1) {
            printf("Chọn chức năng [0-7]: ");
            if (!fgets(buf, sizeof(buf), stdin)) return 0;
            clean_newline(buf);

            if (strlen(buf) == 0) continue;

            choice = strtol(buf, &endptr, 10);

            if (*endptr != '\0' || choice < 0 || choice > 7) {
                printf(">> Lỗi: Lựa chọn '%s' không hợp lệ! Vui lòng chỉ nhập số từ 0 đến 7.\n", buf);
            } else {
                break;
            }
        }

        switch (choice) {
            case 1: select_or_change_file(); break;
            case 2: show_file_info(); break;
            case 3: show_directory_tree(); break;
            case 4: move_file(); break;
            case 5: copy_file(); break;
            case 6: delete_file(); break;
            case 7: search_file(); break;
            case 0:
                printf("\nĐã thoát chương trình.\n");
                return 0;
        }
    }
    return 0;
}
