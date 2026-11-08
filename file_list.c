#define _GNU_SOURCE
#include "file_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

void file_list_init(FileList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void file_list_free(FileList *list) {
    if (list->items) {
        free(list->items);
        list->items = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

static SortMode g_sort_mode = SORT_NAME_ASC;

static int compare_items(const void *a, const void *b) {
    const FileItem *ia = (const FileItem *)a;
    const FileItem *ib = (const FileItem *)b;

    // ".." luôn nằm trên cùng
    if (strcmp(ia->name, "..") == 0) return -1;
    if (strcmp(ib->name, "..") == 0) return 1;

    // Thư mục luôn đứng trước tệp tin
    if (ia->is_dir && !ib->is_dir) return -1;
    if (!ia->is_dir && ib->is_dir) return 1;

    // Sắp xếp theo chế độ được chọn
    switch (g_sort_mode) {
        case SORT_NAME_ASC:
            return strcasecmp(ia->name, ib->name);
        case SORT_NAME_DESC:
            return strcasecmp(ib->name, ia->name);
        case SORT_SIZE_DESC:
            if (ia->size != ib->size) return (ib->size > ia->size) ? 1 : -1;
            return strcasecmp(ia->name, ib->name);
        case SORT_SIZE_ASC:
            if (ia->size != ib->size) return (ia->size > ib->size) ? 1 : -1;
            return strcasecmp(ia->name, ib->name);
        case SORT_TIME_DESC:
            if (ia->mtime != ib->mtime) return (ib->mtime > ia->mtime) ? 1 : -1;
            return strcasecmp(ia->name, ib->name);
        case SORT_TIME_ASC:
            if (ia->mtime != ib->mtime) return (ia->mtime > ib->mtime) ? 1 : -1;
            return strcasecmp(ia->name, ib->name);
    }
    return strcasecmp(ia->name, ib->name);
}

const char* sort_mode_name(SortMode mode) {
    switch (mode) {
        case SORT_NAME_ASC: return "Tên A-Z";
        case SORT_NAME_DESC: return "Tên Z-A";
        case SORT_SIZE_DESC: return "Dung lượng ↓";
        case SORT_SIZE_ASC: return "Dung lượng ↑";
        case SORT_TIME_DESC: return "Mới nhất";
        case SORT_TIME_ASC: return "Cũ nhất";
        default: return "Mặc định";
    }
}

int file_list_load(FileList *list, const char *dir_path, const char *filter, int show_hidden, SortMode sort_mode) {
    g_sort_mode = sort_mode;
    file_list_free(list);

    DIR *dir = opendir(dir_path);
    if (!dir) return -1;

    list->capacity = 64;
    list->items = malloc(list->capacity * sizeof(FileItem));
    if (!list->items) {
        closedir(dir);
        return -1;
    }

    // Luôn thêm thư mục cha ".." nếu không phải là gốc "/"
    if (strcmp(dir_path, "/") != 0) {
        FileItem parent;
        memset(&parent, 0, sizeof(parent));
        safe_copy_str(parent.name, "..", sizeof(parent.name));
        snprintf(parent.full_path, sizeof(parent.full_path), "%s/..", dir_path);
        parent.is_dir = 1;
        parent.mode = S_IFDIR | 0755;
        list->items[list->count++] = parent;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Bỏ qua tệp ẩn nếu không bật hiển thị
        if (!show_hidden && entry->d_name[0] == '.')
            continue;

        // Bỏ qua nếu không khớp từ khóa bộ lọc
        if (filter && strlen(filter) > 0) {
            if (strcasestr(entry->d_name, filter) == NULL)
                continue;
        }

        // Cấp phát mở rộng mảng động khi đầy
        if (list->count >= list->capacity) {
            int new_cap = list->capacity * 2;
            FileItem *new_items = realloc(list->items, new_cap * sizeof(FileItem));
            if (!new_items) break;
            list->items = new_items;
            list->capacity = new_cap;
        }

        FileItem *item = &list->items[list->count];
        memset(item, 0, sizeof(FileItem));
        safe_copy_str(item->name, entry->d_name, sizeof(item->name));
        snprintf(item->full_path, sizeof(item->full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(item->full_path, &st) == 0) {
            item->mode = st.st_mode;
            item->size = st.st_size;
            item->mtime = st.st_mtime;
            item->is_dir = S_ISDIR(st.st_mode);
            item->is_symlink = S_ISLNK(st.st_mode);
            item->is_exec = (!item->is_dir && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)));
        }

        list->count++;
    }

    closedir(dir);

    // Sắp xếp danh sách
    if (list->count > 0) {
        qsort(list->items, list->count, sizeof(FileItem), compare_items);
    }

    return list->count;
}

