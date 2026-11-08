#define _GNU_SOURCE
#include "tui_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>

static struct termios orig_termios;
static int raw_mode_active = 0;

static void handle_signal(int sig) {
    (void)sig;
    tui_cleanup();
    exit(0);
}

void tui_init(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) return;
    raw_mode_active = 1;
    atexit(tui_cleanup);

    // Chuyển sang màn hình phụ (Alternate buffer) và ẩn con trỏ
    printf("\033[?1049h\033[?25l");
    fflush(stdout);
}

void tui_cleanup(void) {
    if (raw_mode_active) {
        // Hiện con trỏ và quay về màn hình terminal chính
        printf("\033[?25h\033[?1049l");
        fflush(stdout);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_active = 0;
    }
}

void tui_get_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        *rows = 24;
        *cols = 80;
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    }
}

void tui_clear(void) {
    printf("\033[2J\033[H");
}

void tui_cursor_move(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

void tui_hide_cursor(void) {
    printf("\033[?25l");
}

void tui_show_cursor(void) {
    printf("\033[?25h");
}

void tui_flush(void) {
    fflush(stdout);
}

void tui_set_color(const char *ansi_code) {
    printf("%s", ansi_code);
}

void tui_reset_color(void) {
    printf(ANSI_RESET);
}

void tui_print(int row, int col, const char *text) {
    tui_cursor_move(row, col);
    printf("%s", text);
}

static size_t utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

void tui_print_len(int row, int col, const char *text, int max_len) {
    if (max_len <= 0) return;
    tui_cursor_move(row, col);

    int printed_cols = 0;
    size_t i = 0;
    size_t len = strlen(text);

    while (i < len && printed_cols < max_len) {
        size_t clen = utf8_char_len((unsigned char)text[i]);
        if (i + clen > len) break;
        if (printed_cols + 1 > max_len) break;

        for (size_t c = 0; c < clen; c++) {
            putchar(text[i + c]);
        }
        i += clen;
        printed_cols++;
    }

    while (printed_cols < max_len) {
        putchar(' ');
        printed_cols++;
    }
}

void tui_fill_line(int row, int col, int width, char ch) {
    tui_cursor_move(row, col);
    for (int i = 0; i < width; i++) {
        putchar(ch);
    }
}

void tui_draw_hline(int row, int left, int width) {
    tui_cursor_move(row, left);
    for (int i = 0; i < width; i++) {
        printf("─");
    }
}

void tui_draw_vline(int col, int top, int height) {
    for (int r = top; r < top + height; r++) {
        tui_cursor_move(r, col);
        printf("│");
    }
}

void tui_draw_box(int top, int left, int width, int height, const char *title) {
    if (width < 3 || height < 3) return;

    // Đường viền trên
    tui_cursor_move(top, left);
    printf("┌");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┐");

    // Đường viền 2 bên
    for (int r = top + 1; r < top + height - 1; r++) {
        tui_cursor_move(r, left);
        printf("│");
        tui_cursor_move(r, left + width - 1);
        printf("│");
    }

    // Đường viền dưới
    tui_cursor_move(top + height - 1, left);
    printf("└");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┘");

    // Tiêu đề khung
    if (title && strlen(title) > 0) {
        int title_len = (int)strlen(title);
        if (title_len > width - 6) title_len = width - 6;
        tui_cursor_move(top, left + 2);
        printf("┤ ");
        tui_set_color(ANSI_BOLD);
        for (int i = 0; i < title_len; i++) putchar(title[i]);
        tui_reset_color();
        printf(" ├");
    }
}

static int wait_for_fd(int fd, int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    return select(fd + 1, &fds, NULL, NULL, &tv);
}

int tui_read_key(void) {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 0) return KEY_EOF;
    if (n < 0) return KEY_NONE;

    if (c == 27) { // Ký tự bắt đầu chuỗi ESC
        if (!wait_for_fd(STDIN_FILENO, 50)) {
            return KEY_ESC; // Phím ESC đơn lẻ
        }

        unsigned char seq[8];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return KEY_ESC;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return KEY_HOME;
                        case '3': return KEY_DELETE;
                        case '4': return KEY_END;
                        case '5': return KEY_PAGE_UP;
                        case '6': return KEY_PAGE_DOWN;
                        case '7': return KEY_HOME;
                        case '8': return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return KEY_UP;
                    case 'B': return KEY_DOWN;
                    case 'C': return KEY_RIGHT;
                    case 'D': return KEY_LEFT;
                    case 'H': return KEY_HOME;
                    case 'F': return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;
            switch (seq[1]) {
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
            }
        }
        return KEY_NONE;
    }

    if (c == 127 || c == 8) return KEY_BACKSPACE;
    if (c == 13 || c == 10) return KEY_ENTER;
    if (c == 9) return KEY_TAB;

    return (int)c;
}

