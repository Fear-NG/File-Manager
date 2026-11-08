#define _GNU_SOURCE
#include "file_search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

static void tree_recursive(const char *base, int depth, int max_depth, char lines[][PREVIEW_LINE_LEN], int max_lines, int *actual_lines) {
    if (depth > max_depth || *actual_lines >= max_lines) return;

    DIR *dir = opendir(base);
    if (!dir) return;

    struct dirent *entry;
    char path[MAX_PATH_LEN * 2];

    while ((entry = readdir(dir)) != NULL && *actual_lines < max_lines) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char indent[128] = "";
        for (int i = 0; i < depth; i++) {
            strncat(indent, "│   ", sizeof(indent) - strlen(indent) - 1);
        }

        snprintf(path, sizeof(path), "%s/%s", base, entry->d_name);
        struct stat st;
        int is_d = 0;
        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) is_d = 1;

        snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "%s├── %s%s", indent, entry->d_name, is_d ? "/" : "");

        if (is_d && depth < max_depth) {
            tree_recursive(path, depth + 1, max_depth, lines, max_lines, actual_lines);
        }
    }
    closedir(dir);
}

void get_dir_tree(const char *dir_path, char lines[][PREVIEW_LINE_LEN], int max_lines, int *actual_lines, int max_depth) {
    *actual_lines = 0;
    snprintf(lines[(*actual_lines)++], PREVIEW_LINE_LEN, "%s", dir_path);
    tree_recursive(dir_path, 0, max_depth, lines, max_lines, actual_lines);
}

void search_files_recursive(const char *base_dir, const char *keyword, SearchResult *results, int max_results, int *actual_results) {
    if (*actual_results >= max_results) return;

    DIR *dir = opendir(base_dir);
    if (!dir) return;

    struct dirent *entry;
    char full_path[MAX_PATH_LEN * 2];

    while ((entry = readdir(dir)) != NULL && *actual_results < max_results) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", base_dir, entry->d_name);
        struct stat st;
        if (lstat(full_path, &st) != 0) continue;

        if (strcasestr(entry->d_name, keyword) != NULL) {
            SearchResult *r = &results[*actual_results];
            safe_copy_str(r->path, full_path, sizeof(r->path));
            safe_copy_str(r->name, entry->d_name, sizeof(r->name));
            r->is_dir = S_ISDIR(st.st_mode);
            r->size = st.st_size;
            (*actual_results)++;
        }

        if (S_ISDIR(st.st_mode)) {
            search_files_recursive(full_path, keyword, results, max_results, actual_results);
        }
    }
    closedir(dir);
}

