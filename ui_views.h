#ifndef UI_VIEWS_H
#define UI_VIEWS_H

#include "file_types.h"

// Vẽ thanh tiêu đề trên cùng (Vị trí thư mục, dung lượng ổ đĩa, chế độ sắp xếp)
void render_header(const AppState *state, int cols);

// Vẽ khung bên trái: Danh sách tệp & thư mục
void render_file_list(AppState *state, int rows, int left_w);

// Vẽ khung bên phải: Chi tiết siêu dữ liệu & Bảng xem trước trực tiếp
void render_preview_pane(AppState *state, int rows, int left_w, int cols);

// Vẽ thanh thông báo trạng thái & bộ nhớ đệm
void render_status_bar(const AppState *state, int rows, int cols);

// Vẽ thanh phím tắt hướng dẫn dưới cùng
void render_hotkeys_bar(int rows, int cols);

#endif // UI_VIEWS_H

