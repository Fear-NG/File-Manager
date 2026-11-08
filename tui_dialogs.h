#ifndef TUI_DIALOGS_H
#define TUI_DIALOGS_H

// Hộp thoại nhập văn bản (Đổi tên, tạo tệp/thư mục, lọc...)
// Trả về 1 nếu xác nhận (Enter), 0 nếu hủy (Esc)
int tui_input_dialog(const char *title, const char *prompt, char *buffer, int max_len);

// Hộp thoại xác nhận Có / Không (Xóa tệp, ghi đè, thoát...)
// Trả về 1 nếu chọn Có (Y / Enter), 0 nếu chọn Không (N / Esc)
int tui_confirm_dialog(const char *title, const char *question);

// Hộp thoại thông báo cảnh báo / lỗi / thông tin
void tui_alert_dialog(const char *title, const char *message);

#endif // TUI_DIALOGS_H

