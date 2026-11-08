#ifndef TUI_CORE_H
#define TUI_CORE_H

#include <termios.h>

// Định nghĩa mã phím đặc biệt
enum {
    KEY_EOF = -1,
    KEY_NONE = 0,
    KEY_UP = 1000,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
    KEY_BACKSPACE = 127,
    KEY_ENTER = 10,
    KEY_ESC = 27,
    KEY_TAB = 9
};

// Mã màu ANSI định dạng
#define ANSI_RESET        "\033[0m"
#define ANSI_BOLD         "\033[1m"
#define ANSI_DIM          "\033[2m"
#define ANSI_REVERSE      "\033[7m"

#define ANSI_FG_BLACK     "\033[30m"
#define ANSI_FG_RED       "\033[31m"
#define ANSI_FG_GREEN     "\033[32m"
#define ANSI_FG_YELLOW    "\033[33m"
#define ANSI_FG_BLUE      "\033[34m"
#define ANSI_FG_MAGENTA   "\033[35m"
#define ANSI_FG_CYAN      "\033[36m"
#define ANSI_FG_WHITE     "\033[37m"

#define ANSI_FG_B_BLACK   "\033[90m" // Xám
#define ANSI_FG_B_RED     "\033[91m"
#define ANSI_FG_B_GREEN   "\033[92m"
#define ANSI_FG_B_YELLOW  "\033[93m"
#define ANSI_FG_B_BLUE    "\033[94m"
#define ANSI_FG_B_MAGENTA "\033[95m"
#define ANSI_FG_B_CYAN    "\033[96m"
#define ANSI_FG_B_WHITE   "\033[97m"

#define ANSI_BG_BLUE      "\033[44m"
#define ANSI_BG_CYAN      "\033[46m"
#define ANSI_BG_GRAY      "\033[100m"
#define ANSI_BG_DARK      "\033[40m"

// Khởi tạo và dọn dẹp terminal
void tui_init(void);
void tui_cleanup(void);

// Kích thước và điều khiển con trỏ
void tui_get_size(int *rows, int *cols);
void tui_clear(void);
void tui_cursor_move(int row, int col);
void tui_hide_cursor(void);
void tui_show_cursor(void);
void tui_flush(void);

// Thiết lập màu sắc
void tui_set_color(const char *ansi_code);
void tui_reset_color(void);

// Vẽ khung viền UTF-8 và in chữ
void tui_draw_box(int top, int left, int width, int height, const char *title);
void tui_draw_hline(int row, int left, int width);
void tui_draw_vline(int col, int top, int height);
void tui_print(int row, int col, const char *text);
void tui_print_len(int row, int col, const char *text, int max_len);
void tui_fill_line(int row, int col, int width, char ch);

// Đọc phím bấm
int tui_read_key(void);

#endif // TUI_CORE_H

