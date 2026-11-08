#define _GNU_SOURCE
#include "modals.h"
#include "tui_core.h"
#include "tui_dialogs.h"
#include "file_crud.h"
#include "file_preview.h"
#include "file_search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void show_help_modal(void) {
    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = 68;
    int height = 23;
    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    for (int r = top; r < top + height; r++) {
        tui_fill_line(r, left, width, ' ');
    }

    tui_set_color(ANSI_FG_B_CYAN);
    tui_draw_box(top, left, width, height, "BẢNG PHÍM TẮT ĐIỀU KHIỂN (HELP)");
    tui_reset_color();

    const char *shortcuts[] = {
        " ↑ / ↓            : Di chuyển chọn tệp / thư mục",
        " Enter / →        : Mở thư mục / Xem chi tiết tệp",
        " ← / Backspace    : Trở về thư mục cha (..)",
        " Space            : Đọc toàn màn hình tệp đang chọn (Pager)",
        " c / C            : Sao chép (Copy) tệp/thư mục đang chọn",
        " x / X            : Cắt (Cut) tệp/thư mục đang chọn",
        " v / V            : Dán (Paste) vào thư mục hiện tại",
        " r                : Đổi tên (Rename) tệp/thư mục",
        " d / Delete       : Xóa tệp/thư mục (có hộp thoại hỏi)",
        " n                : Tạo tệp rỗng mới (New File)",
        " m                : Tạo thư mục mới (New Folder)",
        " /                : Lọc nhanh danh sách trong thư mục hiện tại",
        " o                : Chuyển chế độ sắp xếp (Tên, Size, Thời gian)",
        " .                : Bật / Tắt hiển thị tệp ẩn (.dotfiles)",
        " s                : Tìm kiếm đệ quy toàn diện & nhảy tới tệp",
        " t                : Xem sơ đồ cây thư mục (Tree View)",
        " i                : Xem thông tin chi tiết / siêu dữ liệu",
        " q / Esc          : Thoát chương trình / Đóng cửa sổ"
    };

    int num_lines = sizeof(shortcuts) / sizeof(shortcuts[0]);
    for (int i = 0; i < num_lines && i < height - 4; i++) {
        tui_cursor_move(top + 2 + i, left + 2);
        printf("%s", shortcuts[i]);
    }

    tui_cursor_move(top + height - 2, left + (width - 24) / 2);
    tui_set_color(ANSI_FG_B_YELLOW ANSI_BOLD);
    printf("[ Nhấn phím bất kỳ để đóng ]");
    tui_reset_color();
    tui_flush();

    tui_read_key();
}

void show_info_modal(const FileItem *item) {
    if (!item) return;

    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = 68;
    int height = 18;
    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    for (int r = top; r < top + height; r++) {
        tui_fill_line(r, left, width, ' ');
    }

    tui_set_color(ANSI_FG_B_GREEN);
    tui_draw_box(top, left, width, height, "THÔNG TIN CHI TIẾT TỆP / THƯ MỤC");
    tui_reset_color();

    char size_str[64];
    file_format_size(item->size, size_str, sizeof(size_str));

    char time_str[64];
    file_format_time(item->mtime, time_str, sizeof(time_str));

    char perm_str[16];
    file_format_perms(item->mode, perm_str, sizeof(perm_str));

    const char *type_desc = "Tệp tin thông thường";
    if (item->is_dir) type_desc = "Thư mục (Directory)";
    else if (item->is_symlink) type_desc = "Liên kết mềm (Symlink)";
    else if (item->is_exec) type_desc = "Tệp thực thi (Executable)";

    char row_buf[MAX_PATH_LEN + 128];

    snprintf(row_buf, sizeof(row_buf), "Tên       : %s", item->name);
    tui_print_len(top + 2, left + 3, row_buf, width - 6);

    snprintf(row_buf, sizeof(row_buf), "Loại      : %s", type_desc);
    tui_print_len(top + 3, left + 3, row_buf, width - 6);

    snprintf(row_buf, sizeof(row_buf), "Đường dẫn : %s", item->full_path);
    tui_print_len(top + 4, left + 3, row_buf, width - 6);

    snprintf(row_buf, sizeof(row_buf), "Dung lượng: %s (%ld bytes)", size_str, (long)item->size);
    tui_print_len(top + 5, left + 3, row_buf, width - 6);

    snprintf(row_buf, sizeof(row_buf), "Quyền hạn : %s (%04o)", perm_str, item->mode & 07777);
    tui_print_len(top + 6, left + 3, row_buf, width - 6);

    snprintf(row_buf, sizeof(row_buf), "Sửa đổi   : %s", time_str);
    tui_print_len(top + 7, left + 3, row_buf, width - 6);

    tui_draw_hline(top + 9, left + 2, width - 4);

    if (has_extension(item->full_path, ".pdf")) {
        tui_print_len(top + 10, left + 3, "Định dạng PDF: Hỗ trợ trích xuất thông tin qua pdfinfo/pdftotext", width - 6);
    } else if (has_extension(item->full_path, ".docx")) {
        tui_print_len(top + 10, left + 3, "Định dạng DOCX: Hỗ trợ trích xuất siêu dữ liệu qua exiftool/docx2txt", width - 6);
    } else if (item->is_dir) {
        tui_print_len(top + 10, left + 3, "Nhấn Enter để truy cập vào duyệt nội dung thư mục này.", width - 6);
    } else {
        tui_print_len(top + 10, left + 3, "Tệp văn bản: Nhấn [Space] để đọc toàn màn hình.", width - 6);
    }

    tui_cursor_move(top + height - 2, left + (width - 24) / 2);
    tui_set_color(ANSI_FG_B_YELLOW ANSI_BOLD);
    printf("[ Nhấn Enter/Esc để đóng ]");
    tui_reset_color();
    tui_flush();

    while (1) {
        int k = tui_read_key();
        if (k == KEY_ENTER || k == KEY_ESC || k == ' ' || k == 'q' || k == 'Q' || k == KEY_EOF) break;
    }
}

void show_tree_modal(const char *current_dir) {
    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = cols - 10;
    if (width > 80) width = 80;
    if (width < 40) width = 40;

    int height = rows - 6;
    if (height > 26) height = 26;
    if (height < 12) height = 12;

    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    char tree_lines[256][PREVIEW_LINE_LEN];
    int actual_lines = 0;
    get_dir_tree(current_dir, tree_lines, 256, &actual_lines, 3);

    int scroll_offset = 0;
    int max_visible = height - 4;

    while (1) {
        for (int r = top; r < top + height; r++) {
            tui_fill_line(r, left, width, ' ');
        }

        tui_set_color(ANSI_FG_B_CYAN);
        tui_draw_box(top, left, width, height, "CÂY THƯ MỤC (DIRECTORY TREE)");
        tui_reset_color();

        for (int i = 0; i < max_visible; i++) {
            int line_idx = scroll_offset + i;
            if (line_idx < actual_lines) {
                tui_print_len(top + 2 + i, left + 2, tree_lines[line_idx], width - 4);
            }
        }

        char footer[64];
        snprintf(footer, sizeof(footer), "[ %d/%d dòng - ↑↓/PgUp/PgDn cuộn, Esc đóng ]", 
                 actual_lines > 0 ? scroll_offset + 1 : 0, actual_lines);
        tui_cursor_move(top + height - 1, left + (width - (int)strlen(footer)) / 2);
        tui_set_color(ANSI_FG_B_YELLOW);
        printf("%s", footer);
        tui_reset_color();
        tui_flush();

        int k = tui_read_key();
        if (k == KEY_ESC || k == KEY_ENTER || k == 'q' || k == 'Q' || k == KEY_EOF) {
            break;
        } else if (k == KEY_UP) {
            if (scroll_offset > 0) scroll_offset--;
        } else if (k == KEY_DOWN) {
            if (scroll_offset + max_visible < actual_lines) scroll_offset++;
        } else if (k == KEY_PAGE_UP) {
            scroll_offset -= max_visible;
            if (scroll_offset < 0) scroll_offset = 0;
        } else if (k == KEY_PAGE_DOWN) {
            scroll_offset += max_visible;
            if (scroll_offset + max_visible > actual_lines) {
                scroll_offset = actual_lines - max_visible;
                if (scroll_offset < 0) scroll_offset = 0;
            }
        } else if (k == KEY_HOME) {
            scroll_offset = 0;
        } else if (k == KEY_END) {
            scroll_offset = actual_lines - max_visible;
            if (scroll_offset < 0) scroll_offset = 0;
        }
    }
}

int show_search_modal(const char *current_dir, char *chosen_path, int *chosen_is_dir) {
    char keyword[128] = "";
    if (!tui_input_dialog("TÌM KIẾM ĐỆ QUY", "Nhập từ khóa hoặc đuôi mở rộng (VD: demo, .txt, .c):", keyword, sizeof(keyword))) {
        return 0;
    }

    if (strlen(keyword) == 0) {
        tui_alert_dialog("THÔNG BÁO", "Từ khóa tìm kiếm không được để trống!");
        return 0;
    }

    SearchResult results[200];
    int total_results = 0;
    search_files_recursive(current_dir, keyword, results, 200, &total_results);

    if (total_results == 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Không tìm thấy kết quả nào khớp với '%s'!", keyword);
        tui_alert_dialog("KẾT QUẢ TÌM KIẾM", msg);
        return 0;
    }

    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = cols - 12;
    if (width > 86) width = 86;
    if (width < 50) width = 50;

    int height = rows - 6;
    if (height > 24) height = 24;
    if (height < 12) height = 12;

    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    int selected = 0;
    int scroll_offset = 0;
    int max_visible = height - 5;

    char title[256];
    snprintf(title, sizeof(title), "KẾT QUẢ TÌM: '%s' (%d mục)", keyword, total_results);

    while (1) {
        if (selected < scroll_offset) scroll_offset = selected;
        if (selected >= scroll_offset + max_visible) scroll_offset = selected - max_visible + 1;

        for (int r = top; r < top + height; r++) {
            tui_fill_line(r, left, width, ' ');
        }

        tui_set_color(ANSI_FG_B_YELLOW);
        tui_draw_box(top, left, width, height, title);
        tui_reset_color();

        tui_print_len(top + 1, left + 2, "   LOẠI   DUNG LƯỢNG   ĐƯỜNG DẪN", width - 4);
        tui_draw_hline(top + 2, left + 2, width - 4);

        for (int i = 0; i < max_visible; i++) {
            int idx = scroll_offset + i;
            if (idx >= total_results) break;

            int row = top + 3 + i;
            int is_sel = (idx == selected);

            char size_str[32];
            file_format_size(results[idx].size, size_str, sizeof(size_str));

            char line_text[MAX_PATH_LEN + 64];
            snprintf(line_text, sizeof(line_text), "  [%-4s] %-10s  %s",
                     results[idx].is_dir ? "DIR" : "FILE",
                     results[idx].is_dir ? "-" : size_str,
                     results[idx].path);

            if (is_sel) {
                tui_set_color(ANSI_REVERSE ANSI_BOLD ANSI_FG_B_CYAN);
            } else if (results[idx].is_dir) {
                tui_set_color(ANSI_FG_B_BLUE ANSI_BOLD);
            }
            tui_print_len(row, left + 2, line_text, width - 4);
            tui_reset_color();
        }

        tui_cursor_move(top + height - 2, left + 3);
        tui_set_color(ANSI_FG_B_GREEN);
        printf("[Enter] Chuyển tới vị trí mục   [Esc] Hủy bỏ   [↑↓] Di chuyển");
        tui_reset_color();
        tui_flush();

        int k = tui_read_key();
        if (k == KEY_ESC || k == 'q' || k == 'Q' || k == KEY_EOF) {
            return 0;
        } else if (k == KEY_ENTER) {
            safe_copy_str(chosen_path, results[selected].path, MAX_PATH_LEN);
            *chosen_is_dir = results[selected].is_dir;
            return 1;
        } else if (k == KEY_UP) {
            if (selected > 0) selected--;
        } else if (k == KEY_DOWN) {
            if (selected < total_results - 1) selected++;
        } else if (k == KEY_PAGE_UP) {
            selected -= max_visible;
            if (selected < 0) selected = 0;
        } else if (k == KEY_PAGE_DOWN) {
            selected += max_visible;
            if (selected >= total_results) selected = total_results - 1;
        } else if (k == KEY_HOME) {
            selected = 0;
        } else if (k == KEY_END) {
            selected = total_results - 1;
        }
    }
}

void show_pager_modal(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        tui_alert_dialog("LỖI", "Không thể mở tệp tin để đọc!");
        return;
    }

    #define PAGER_MAX_LINES 1500
    char (*lines)[PREVIEW_LINE_LEN] = malloc(PAGER_MAX_LINES * sizeof(*lines));
    if (!lines) {
        fclose(fp);
        return;
    }

    int total_lines = 0;
    char buf[PREVIEW_LINE_LEN];
    while (fgets(buf, sizeof(buf), fp) && total_lines < PAGER_MAX_LINES) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        snprintf(lines[total_lines], PREVIEW_LINE_LEN, "%4d │ %s", total_lines + 1, buf);
        total_lines++;
    }
    fclose(fp);

    if (total_lines == 0) {
        snprintf(lines[total_lines++], PREVIEW_LINE_LEN, "(Tệp tin rỗng)");
    }

    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = cols - 4;
    int height = rows - 2;
    int top = 2;
    int left = 3;

    int scroll_offset = 0;
    int max_visible = height - 4;

    char box_title[256];
    snprintf(box_title, sizeof(box_title), "TRÌNH ĐỌC TỆP: %s", filepath);

    while (1) {
        for (int r = top; r < top + height; r++) {
            tui_fill_line(r, left, width, ' ');
        }

        tui_set_color(ANSI_FG_B_WHITE);
        tui_draw_box(top, left, width, height, box_title);
        tui_reset_color();

        for (int i = 0; i < max_visible; i++) {
            int idx = scroll_offset + i;
            if (idx < total_lines) {
                tui_print_len(top + 2 + i, left + 2, lines[idx], width - 4);
            }
        }

        int percent = (total_lines > 0) ? ((scroll_offset + max_visible) * 100) / total_lines : 100;
        if (percent > 100) percent = 100;

        char footer[128];
        snprintf(footer, sizeof(footer), "[ Dòng %d-%d / %d (%d%%) - ↑↓/PgUp/PgDn cuộn, Space/Esc đóng ]",
                 scroll_offset + 1,
                 (scroll_offset + max_visible > total_lines) ? total_lines : scroll_offset + max_visible,
                 total_lines, percent);

        tui_cursor_move(top + height - 1, left + (width - (int)strlen(footer)) / 2);
        tui_set_color(ANSI_FG_B_YELLOW ANSI_BOLD);
        printf("%s", footer);
        tui_reset_color();
        tui_flush();

        int k = tui_read_key();
        if (k == KEY_ESC || k == ' ' || k == 'q' || k == 'Q' || k == KEY_EOF) {
            break;
        } else if (k == KEY_UP) {
            if (scroll_offset > 0) scroll_offset--;
        } else if (k == KEY_DOWN) {
            if (scroll_offset + max_visible < total_lines) scroll_offset++;
        } else if (k == KEY_PAGE_UP) {
            scroll_offset -= max_visible;
            if (scroll_offset < 0) scroll_offset = 0;
        } else if (k == KEY_PAGE_DOWN) {
            scroll_offset += max_visible;
            if (scroll_offset + max_visible > total_lines) {
                scroll_offset = total_lines - max_visible;
                if (scroll_offset < 0) scroll_offset = 0;
            }
        } else if (k == KEY_HOME) {
            scroll_offset = 0;
        } else if (k == KEY_END) {
            scroll_offset = total_lines - max_visible;
            if (scroll_offset < 0) scroll_offset = 0;
        }
    }

    free(lines);
}
