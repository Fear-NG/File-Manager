#define _GNU_SOURCE
#include "file_crud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <fcntl.h>
#include <dirent.h>

void file_format_size(off_t bytes, char *out, size_t out_sz) {
    if (bytes < 1024) {
        snprintf(out, out_sz, "%ld B", (long)bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(out, out_sz, "%.1f KB", (double)bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(out, out_sz, "%.1f MB", (double)bytes / (1024.0 * 1024.0));
    } else {
        snprintf(out, out_sz, "%.1f GB", (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

void file_format_time(time_t mtime, char *out, size_t out_sz) {
    struct tm *tm_info = localtime(&mtime);
    if (!tm_info) {
        safe_copy_str(out, "N/A", out_sz);
        return;
    }
    strftime(out, out_sz, "%Y-%m-%d %H:%M", tm_info);
}

void file_format_perms(mode_t mode, char *out, size_t out_sz) {
    if (out_sz < 11) return;
    out[0] = S_ISDIR(mode) ? 'd' : (S_ISLNK(mode) ? 'l' : '-');
    out[1] = (mode & S_IRUSR) ? 'r' : '-';
    out[2] = (mode & S_IWUSR) ? 'w' : '-';
    out[3] = (mode & S_IXUSR) ? 'x' : '-';
    out[4] = (mode & S_IRGRP) ? 'r' : '-';
    out[5] = (mode & S_IWGRP) ? 'w' : '-';
    out[6] = (mode & S_IXGRP) ? 'x' : '-';
    out[7] = (mode & S_IROTH) ? 'r' : '-';
    out[8] = (mode & S_IWOTH) ? 'w' : '-';
    out[9] = (mode & S_IXOTH) ? 'x' : '-';
    out[10] = '\0';
}

int get_disk_space(const char *path, char *free_out, size_t free_sz, char *total_out, size_t total_sz) {
    struct statvfs vfs;
    if (statvfs(path, &vfs) != 0) return -1;
    unsigned long long free_bytes = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
    unsigned long long total_bytes = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
    file_format_size((off_t)free_bytes, free_out, free_sz);
    file_format_size((off_t)total_bytes, total_out, total_sz);
    return 0;
}

static int copy_single_file(const char *src, const char *dst) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd < 0) return -1;

    struct stat st;
    if (fstat(src_fd, &st) != 0) {
        close(src_fd);
        return -1;
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);
    if (dst_fd < 0) {
        close(src_fd);
        return -1;
    }

    char buf[65536];
    ssize_t n;
    int success = 1;

    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
        if (write(dst_fd, buf, n) != n) {
            success = 0;
            break;
        }
    }
    if (n < 0) success = 0;

    close(src_fd);
    close(dst_fd);
    return success ? 0 : -1;
}

int file_copy(const char *src, const char *dst) {
    struct stat st;
    if (stat(src, &st) != 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        if (mkdir(dst, st.st_mode & 0777) != 0 && errno != EEXIST) return -1;

        DIR *dir = opendir(src);
        if (!dir) return -1;

        struct dirent *entry;
        char sub_src[MAX_PATH_LEN * 2];
        char sub_dst[MAX_PATH_LEN * 2];
        int res = 0;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(sub_src, sizeof(sub_src), "%s/%s", src, entry->d_name);
            snprintf(sub_dst, sizeof(sub_dst), "%s/%s", dst, entry->d_name);

            if (file_copy(sub_src, sub_dst) != 0) {
                res = -1;
                break;
            }
        }
        closedir(dir);
        return res;
    } else {
        return copy_single_file(src, dst);
    }
}

int file_move(const char *src, const char *dst) {
    if (rename(src, dst) == 0) return 0;
    if (errno == EXDEV) {
        if (file_copy(src, dst) == 0) {
            return file_delete_recursive(src);
        }
    }
    return -1;
}

int file_delete_recursive(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;

        struct dirent *entry;
        char sub_path[MAX_PATH_LEN * 2];
        int res = 0;

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(sub_path, sizeof(sub_path), "%s/%s", path, entry->d_name);
            if (file_delete_recursive(sub_path) != 0) {
                res = -1;
                break;
            }
        }
        closedir(dir);
        if (res == 0) {
            return rmdir(path);
        }
        return -1;
    } else {
        return unlink(path);
    }
}

int file_mkdir(const char *path) {
    return mkdir(path, 0755);
}

int file_touch(const char *path) {
    int fd = open(path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) return -1;
    close(fd);
    return 0;
}

