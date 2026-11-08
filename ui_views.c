#define _GNU_SOURCE
#include "ui_views.h"
#include "tui_core.h"
#include "file_crud.h"
#include "file_preview.h"
#include "file_list.h"
#include <stdio.h>
#include <string.h>

void render_header(const AppState *state, int cols) {
    tui_cursor_move(1, 1);
    tui_set_color(ANSI_BG_BLUE ANSI_FG_B_WHITE ANSI_BOLD);

    char title[] = " 📁 QUẢN LÝ TỆP ";
    printf("%s", title);

    char disk_info[64] = "";
    char free_sp[32], tot_sp[32];
    if (get_disk_space(state->current_dir, free_sp, sizeof(free_sp), tot_sp, sizeof(tot_sp)) == 0) {
        snprintf(disk_info, sizeof(disk_info), "[Trống: %s] ", free_sp);
    }

    char sort_info[64];
    snprintf(sort_info, sizeof(sort_info), "[%s%s] ",
             sort_mode_name(state->sort_mode),
             state->show_hidden ? " +Ẩn" : "");

    char path_buf[MAX_PATH_LEN * 2];
    snprintf(path_buf, sizeof(path_buf), " %s%sVị trí: %s ", disk_info, sort_info, state->current_dir);

    int remaining = cols - (int)strlen(title);
    if (remaining > 0) {
        int path_len = (int)strlen(path_buf);
        if (path_len > remaining) {
            // Rút gọn đường dẫn nếu quá dài
            const char *short_path = state->current_dir + (path_len - remaining + 6);
            printf(" .../%s ", short_path);
            int cur_printed = 6 + (int)strlen(short_path);
            for (int i = cur_printed; i < remaining; i++) putchar(' ');
        } else {
            printf("%s", path_buf);
            for (int i = path_len; i < remaining; i++) putchar(' ');
        }
    }
    tui_reset_color();
}

void render_file_list(AppState *state, int rows, int left_w) {
    int top = 2;
    int height = rows - 4;
    int max_visible = height - 2;

    if (state->selected_index < state->scroll_offset) {
        state->scroll_offset = state->selected_index;
    }
    if (state->selected_index >= state->scroll_offset + max_visible) {
        state->scroll_offset = state->selected_index - max_visible + 1;
    }
    if (state->scroll_offset < 0) state->scroll_offset = 0;

    char box_title[256];
    if (strlen(state->filter) > 0) {
        snprintf(box_title, sizeof(box_title), "DANH SÁCH TỆP (Lọc: '%s' - %d mục)", state->filter, state->file_list.count);
    } else {
        snprintf(box_title, sizeof(box_title), "DANH SÁCH TỆP (%d mục)", state->file_list.count);
    }

    tui_set_color(ANSI_FG_B_BLUE);
    tui_draw_box(top, 1, left_w, height, box_title);
    tui_reset_color();

    for (int i = 0; i < max_visible; i++) {
        int idx = state->scroll_offset + i;
        int row = top + 1 + i;

        if (idx >= state->file_list.count) {
            tui_fill_line(row, 2, left_w - 2, ' ');
            continue;
        }

        const FileItem *item = &state->file_list.items[idx];
        int is_sel = (idx == state->selected_index);

        char size_str[32] = "";
        if (!item->is_dir && strcmp(item->name, "..") != 0) {
            file_format_size(item->size, size_str, sizeof(size_str));
        }

        // Biểu tượng theo loại tệp
        const char *icon = "📄 ";
        if (strcmp(item->name, "..") == 0) icon = "⤴️ ";
        else if (item->is_dir) icon = "📁 ";
        else if (item->is_symlink) icon = "🔗 ";
        else if (item->is_exec) icon = "⚡ ";
        else if (has_extension(item->name, ".pdf")) icon = "📕 ";
        else if (has_extension(item->name, ".docx")) icon = "📘 ";
        else if (has_extension(item->name, ".c") || has_extension(item->name, ".h")) icon = "🔧 ";

        int size_w = 9;
        int name_max_w = left_w - size_w - 7;
        if (name_max_w < 8) name_max_w = 8;

        tui_cursor_move(row, 2);
        if (is_sel) {
            tui_set_color(ANSI_REVERSE ANSI_BOLD ANSI_FG_B_CYAN);
            printf(" > %s", icon);
        } else {
            if (item->is_dir) {
                tui_set_color(ANSI_FG_B_BLUE ANSI_BOLD);
            } else if (item->is_exec) {
                tui_set_color(ANSI_FG_B_GREEN ANSI_BOLD);
            } else if (has_extension(item->name, ".pdf") || has_extension(item->name, ".docx")) {
                tui_set_color(ANSI_FG_B_YELLOW);
            } else {
                tui_set_color(ANSI_FG_WHITE);
            }
            printf("   %s", icon);
        }

        // In tên tệp cắt gọn vừa chiều rộng khung
        int printed_chars = 0;
        size_t nlen = strlen(item->name);
        for (size_t c = 0; c < nlen && printed_chars < name_max_w; c++) {
            putchar(item->name[c]);
            printed_chars++;
        }
        while (printed_chars < name_max_w) {
            putchar(' ');
            printed_chars++;
        }

        printf("%*s ", size_w, size_str);
        tui_reset_color();
    }
}

void render_preview_pane(AppState *state, int rows, int left_w, int cols) {
    int top = 2;
    int right_left = left_w + 1;
    int right_w = cols - left_w;
    int height = rows - 4;

    tui_set_color(ANSI_FG_B_CYAN);
    tui_draw_box(top, right_left, right_w, height, "THÔNG TIN & XEM TRƯỚC");
    tui_reset_color();

    if (state->file_list.count == 0 || state->selected_index >= state->file_list.count) {
        tui_print_len(top + 2, right_left + 2, "(Thư mục trống)", right_w - 4);
        for (int r = top + 3; r < top + height - 1; r++) {
            tui_fill_line(r, right_left + 1, right_w - 2, ' ');
        }
        return;
    }

    const FileItem *item = &state->file_list.items[state->selected_index];

    // Tự động nạp bộ đệm xem trước khi chuyển tệp
    if (state->cached_preview_index != state->selected_index ||
        strcmp(state->cached_path, item->full_path) != 0) {
        state->cached_preview_index = state->selected_index;
        safe_copy_str(state->cached_path, item->full_path, sizeof(state->cached_path));
        get_file_preview(item->full_path, state->preview_lines, PREVIEW_MAX_LINES, &state->preview_line_count);
    }

    char meta_buf[512];
    char size_str[32];
    char time_str[32];
    char perm_str[16];

    file_format_size(item->size, size_str, sizeof(size_str));
    file_format_time(item->mtime, time_str, sizeof(time_str));
    file_format_perms(item->mode, perm_str, sizeof(perm_str));

    snprintf(meta_buf, sizeof(meta_buf), " Tên : %s", item->name);
    tui_set_color(ANSI_BOLD);
    tui_print_len(top + 1, right_left + 2, meta_buf, right_w - 4);
    tui_reset_color();

    snprintf(meta_buf, sizeof(meta_buf), " Loại: %s   Dung lượng: %s",
             item->is_dir ? "Thư mục" : "Tệp thường", size_str);
    tui_print_len(top + 2, right_left + 2, meta_buf, right_w - 4);

    snprintf(meta_buf, sizeof(meta_buf), " Quyền: %s (%04o)   Sửa: %s",
             perm_str, item->mode & 07777, time_str);
    tui_print_len(top + 3, right_left + 2, meta_buf, right_w - 4);

    tui_draw_hline(top + 4, right_left + 1, right_w - 2);

    int preview_top = top + 5;
    int preview_lines_avail = height - 6;

    for (int i = 0; i < preview_lines_avail; i++) {
        int r = preview_top + i;
        if (i < state->preview_line_count) {
            tui_print_len(r, right_left + 2, state->preview_lines[i], right_w - 4);
        } else {
            tui_fill_line(r, right_left + 1, right_w - 2, ' ');
        }
    }
}

void render_status_bar(const AppState *state, int rows, int cols) {
    (void)cols;
    tui_cursor_move(rows - 2, 1);

    if (state->clip_op != CLIP_NONE) {
        tui_set_color(ANSI_BG_GRAY ANSI_FG_B_YELLOW ANSI_BOLD);
        if (state->clip_op == CLIP_COPY) printf(" [COPY: %s] ", state->clip_name);
        else printf(" [CUT: %s] ", state->clip_name);
        tui_reset_color();
        printf(" ");
    }

    if (strlen(state->status_msg) > 0 && time(NULL) <= state->status_expire) {
        if (state->status_type == 1) tui_set_color(ANSI_FG_B_GREEN ANSI_BOLD);
        else if (state->status_type == 2) tui_set_color(ANSI_FG_B_RED ANSI_BOLD);
        else tui_set_color(ANSI_FG_B_YELLOW);
        printf(">> %s", state->status_msg);
        tui_reset_color();
    }

    printf("\033[K");
}

void render_hotkeys_bar(int rows, int cols) {
    tui_cursor_move(rows - 1, 1);
    tui_set_color(ANSI_BG_DARK ANSI_FG_B_WHITE);

    const char *hints = " [Enter]Mở [c]Chép [x]Cắt [v]Dán [r]Đổi tên [d]Xóa [n]Tệp [m]Thư mục [/]Lọc [o]Sắp xếp [.]Tệp ẩn [s]Tìm [t]Cây [Space]Đọc [?]Giúp [q]Thoát";
    int printed = 0;
    size_t len = strlen(hints);
    for (size_t i = 0; i < len && printed < cols; i++) {
        putchar(hints[i]);
        printed++;
    }
    while (printed < cols) {
        putchar(' ');
        printed++;
    }
    tui_reset_color();
}
