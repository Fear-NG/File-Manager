#define _GNU_SOURCE
#include "file_preview.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

int has_extension(const char *filepath, const char *ext) {
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return 0;
    return strcasecmp(dot, ext) == 0;
}

static void clean_string(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}

static void sanitize_line(char *dest, const char *src, size_t max_len) {
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d + 4 < max_len; s++) {
        if (src[s] == '\t') {
            dest[d++] = ' ';
            dest[d++] = ' ';
            dest[d++] = ' ';
            dest[d++] = ' ';
        } else if ((unsigned char)src[s] < 32 && src[s] != '\n') {
            dest[d++] = ' ';
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
    clean_string(dest);
}

void get_file_preview(const char *filepath, char lines[][PREVIEW_LINE_LEN], int max_lines, int *actual_lines) {
    *actual_lines = 0;
    struct stat st;
    if (stat(filepath, &st) != 0) {
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "[Không thể đọc thông tin file]");
        return;
    }

    // 1. Nếu là thư mục: xem trước danh sách tệp bên trong
    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(filepath);
        if (!dir) {
            snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "[Không thể mở thư mục]");
            return;
        }
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "--- NỘI DUNG THƯ MỤC ---");
        struct dirent *entry;
        int count = 0;
        while ((entry = readdir(dir)) != NULL && *actual_lines < max_lines - 1) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            char subpath[MAX_PATH_LEN * 2];
            snprintf(subpath, sizeof(subpath), "%s/%s", filepath, entry->d_name);
            struct stat sst;
            int is_d = 0;
            if (stat(subpath, &sst) == 0 && S_ISDIR(sst.st_mode)) is_d = 1;

            snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, " %s %s", is_d ? "[DIR ]" : "[FILE]", entry->d_name);
            count++;
        }
        closedir(dir);
        if (count == 0) {
            snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, " (Thư mục trống)");
        }
        return;
    }

    // 2. Nếu là PDF: trích xuất metadata và nội dung qua pdfinfo & pdftotext
    if (has_extension(filepath, ".pdf")) {
        char cmd[MAX_PATH_LEN * 2];
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "--- THÔNG TIN PDF (pdfinfo) ---");
        snprintf(cmd, sizeof(cmd), "pdfinfo \"%s\" 2>/dev/null", filepath);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[PREVIEW_LINE_LEN];
            int p_count = 0;
            while (fgets(buf, sizeof(buf), fp) && *actual_lines < max_lines - 5 && p_count < 8) {
                sanitize_line(lines[(*actual_lines)++], buf, PREVIEW_LINE_LEN);
                p_count++;
            }
            pclose(fp);
        }

        if (*actual_lines < max_lines - 2) {
            snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "--- NỘI DUNG TRÍCH XUẤT (pdftotext) ---");
            snprintf(cmd, sizeof(cmd), "pdftotext \"%s\" - 2>/dev/null", filepath);
            fp = popen(cmd, "r");
            if (fp) {
                char buf[PREVIEW_LINE_LEN];
                while (fgets(buf, sizeof(buf), fp) && *actual_lines < max_lines) {
                    sanitize_line(lines[(*actual_lines)++], buf, PREVIEW_LINE_LEN);
                }
                pclose(fp);
            }
        }
        return;
    }

    // 3. Nếu là DOCX: trích xuất metadata và văn bản qua exiftool & docx2txt
    if (has_extension(filepath, ".docx")) {
        char cmd[MAX_PATH_LEN * 2];
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "--- METADATA DOCX (exiftool) ---");
        snprintf(cmd, sizeof(cmd), "exiftool -Title -Creator -CreateDate -WordCount \"%s\" 2>/dev/null", filepath);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[PREVIEW_LINE_LEN];
            int p_count = 0;
            while (fgets(buf, sizeof(buf), fp) && *actual_lines < max_lines - 5 && p_count < 6) {
                sanitize_line(lines[(*actual_lines)++], buf, PREVIEW_LINE_LEN);
                p_count++;
            }
            pclose(fp);
        }

        if (*actual_lines < max_lines - 2) {
            snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "--- NỘI DUNG TRÍCH XUẤT (docx2txt) ---");
            snprintf(cmd, sizeof(cmd), "docx2txt < \"%s\" 2>/dev/null", filepath);
            fp = popen(cmd, "r");
            if (fp) {
                char buf[PREVIEW_LINE_LEN];
                while (fgets(buf, sizeof(buf), fp) && *actual_lines < max_lines) {
                    sanitize_line(lines[(*actual_lines)++], buf, PREVIEW_LINE_LEN);
                }
                pclose(fp);
            }
        }
        return;
    }

    // 4. Tệp tin văn bản hoặc mã nguồn thông thường
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "[Không thể mở file]");
        return;
    }

    // Kiểm tra tệp nhị phân
    char check_buf[512];
    size_t bytes_read = fread(check_buf, 1, sizeof(check_buf), fp);
    int is_binary = 0;
    for (size_t i = 0; i < bytes_read; i++) {
        if (check_buf[i] == '\0') {
            is_binary = 1;
            break;
        }
    }

    if (is_binary) {
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "[Tệp nhị phân - xem mẫu Hex (128 bytes)]");
        rewind(fp);
        unsigned char hex_buf[16];
        int offset = 0;
        while (*actual_lines < max_lines && offset < 128) {
            size_t n = fread(hex_buf, 1, 16, fp);
            if (n == 0) break;
            char line[PREVIEW_LINE_LEN];
            int pos = snprintf(line, sizeof(line), "%04x: ", offset);
            for (size_t i = 0; i < 16; i++) {
                if (i < n) pos += snprintf(line + pos, sizeof(line) - pos, "%02x ", hex_buf[i]);
                else pos += snprintf(line + pos, sizeof(line) - pos, "   ");
            }
            pos += snprintf(line + pos, sizeof(line) - pos, " |");
            for (size_t i = 0; i < n; i++) {
                pos += snprintf(line + pos, sizeof(line) - pos, "%c", isprint(hex_buf[i]) ? hex_buf[i] : '.');
            }
            snprintf(line + pos, sizeof(line) - pos, "|");
            safe_copy_str(lines[(*actual_lines)++], line, PREVIEW_LINE_LEN);
            offset += n;
        }
        fclose(fp);
        return;
    }

    // Xem trước nội dung văn bản thường (có số dòng)
    rewind(fp);
    char buf[PREVIEW_LINE_LEN];
    int line_num = 1;
    while (fgets(buf, sizeof(buf), fp) && *actual_lines < max_lines) {
        char formatted[PREVIEW_LINE_LEN];
        char sanitized[PREVIEW_LINE_LEN - 32];
        sanitize_line(sanitized, buf, sizeof(sanitized));
        snprintf(formatted, sizeof(formatted), "%3d │ %s", line_num++, sanitized);
        safe_copy_str(lines[(*actual_lines)++], formatted, PREVIEW_LINE_LEN);
    }
    fclose(fp);

    if (*actual_lines == 0) {
        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "(File trống)");
    }
}

