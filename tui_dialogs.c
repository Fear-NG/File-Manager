#define _GNU_SOURCE
#include "tui_dialogs.h"
#include "tui_core.h"
#include <stdio.h>
#include <string.h>

int tui_confirm_dialog(const char *title, const char *question) {
    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = 54;
    int height = 7;
    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    for (int r = top; r < top + height; r++) {
        tui_fill_line(r, left, width, ' ');
    }

    tui_set_color(ANSI_FG_B_YELLOW);
    tui_draw_box(top, left, width, height, title);
    tui_reset_color();

    tui_print_len(top + 2, left + 3, question, width - 6);

    tui_cursor_move(top + 4, left + (width - 24) / 2);
    tui_set_color(ANSI_FG_B_GREEN ANSI_BOLD);
    printf("[ Y / Enter: Có ]");
    tui_reset_color();
    printf("   ");
    tui_set_color(ANSI_FG_B_RED ANSI_BOLD);
    printf("[ N / Esc: Không ]");
    tui_reset_color();
    tui_flush();

    while (1) {
        int k = tui_read_key();
        if (k == KEY_EOF) return 0;
        if (k == 'y' || k == 'Y' || k == KEY_ENTER) return 1;
        if (k == 'n' || k == 'N' || k == KEY_ESC || k == 'q' || k == 'Q') return 0;
    }
}

int tui_input_dialog(const char *title, const char *prompt, char *buffer, int max_len) {
    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = 60;
    int height = 7;
    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    int input_len = (int)strlen(buffer);
    int cursor_pos = input_len;

    while (1) {
        for (int r = top; r < top + height; r++) {
            tui_fill_line(r, left, width, ' ');
        }

        tui_set_color(ANSI_FG_B_CYAN);
        tui_draw_box(top, left, width, height, title);
        tui_reset_color();

        tui_print_len(top + 2, left + 3, prompt, width - 6);

        // Khung nhập liệu
        tui_cursor_move(top + 3, left + 3);
        tui_set_color(ANSI_BG_DARK ANSI_FG_B_WHITE);
        int field_w = width - 6;
        for (int i = 0; i < field_w; i++) {
            if (i < input_len) putchar(buffer[i]);
            else putchar(' ');
        }
        tui_reset_color();

        tui_print(top + 5, left + 4, "[Enter] Đồng ý   [ESC] Hủy bỏ");

        // Đặt vị trí con trỏ tại ô nhập
        tui_cursor_move(top + 3, left + 3 + cursor_pos);
        tui_show_cursor();
        tui_flush();

        int k = tui_read_key();
        tui_hide_cursor();

        if (k == KEY_ESC || k == KEY_EOF) {
            return 0; // Hủy thao tác
        } else if (k == KEY_ENTER) {
            return 1; // Xác nhận xong
        } else if (k == KEY_BACKSPACE) {
            if (cursor_pos > 0) {
                memmove(&buffer[cursor_pos - 1], &buffer[cursor_pos], input_len - cursor_pos + 1);
                cursor_pos--;
                input_len--;
            }
        } else if (k == KEY_DELETE) {
            if (cursor_pos < input_len) {
                memmove(&buffer[cursor_pos], &buffer[cursor_pos + 1], input_len - cursor_pos);
                input_len--;
            }
        } else if (k == KEY_LEFT) {
            if (cursor_pos > 0) cursor_pos--;
        } else if (k == KEY_RIGHT) {
            if (cursor_pos < input_len) cursor_pos++;
        } else if (k == KEY_HOME) {
            cursor_pos = 0;
        } else if (k == KEY_END) {
            cursor_pos = input_len;
        } else if (k >= 32 && k < 127 && input_len < max_len - 1 && input_len < field_w - 1) {
            memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], input_len - cursor_pos + 1);
            buffer[cursor_pos] = (char)k;
            cursor_pos++;
            input_len++;
        }
    }
}

void tui_alert_dialog(const char *title, const char *message) {
    int rows, cols;
    tui_get_size(&rows, &cols);

    int width = 56;
    int height = 7;
    int top = (rows - height) / 2;
    int left = (cols - width) / 2;
    if (top < 1) top = 1;
    if (left < 1) left = 1;

    for (int r = top; r < top + height; r++) {
        tui_fill_line(r, left, width, ' ');
    }

    tui_set_color(ANSI_FG_B_RED);
    tui_draw_box(top, left, width, height, title);
    tui_reset_color();

    tui_print_len(top + 2, left + 3, message, width - 6);

    tui_cursor_move(top + 4, left + (width - 18) / 2);
    tui_set_color(ANSI_FG_B_WHITE ANSI_BOLD);
    printf("[ Nhấn Enter/ESC ]");
    tui_reset_color();
    tui_flush();

    while (1) {
        int k = tui_read_key();
        if (k == KEY_ENTER || k == KEY_ESC || k == ' ' || k == 'q' || k == 'Q' || k == KEY_EOF) break;
    }
}

