#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <time.h>
#include <sys/stat.h>

#include "file_types.h"
#include "tui_core.h"
#include "tui_dialogs.h"
#include "file_crud.h"
#include "file_list.h"
#include "ui_views.h"
#include "modals.h"

static void set_status(AppState *state, const char *msg, int type) {
    safe_copy_str(state->status_msg, msg, sizeof(state->status_msg));
    state->status_type = type;
    state->status_expire = time(NULL) + 5;
}

static void reload_directory(AppState *state, const char *select_name) {
    file_list_load(&state->file_list, state->current_dir, state->filter, state->show_hidden, state->sort_mode);

    state->selected_index = 0;
    if (select_name && strlen(select_name) > 0) {
        for (int i = 0; i < state->file_list.count; i++) {
            if (strcmp(state->file_list.items[i].name, select_name) == 0) {
                state->selected_index = i;
                break;
            }
        }
    }

    if (state->selected_index >= state->file_list.count) {
        state->selected_index = state->file_list.count > 0 ? state->file_list.count - 1 : 0;
    }

    state->cached_preview_index = -1;
    state->cached_path[0] = '\0';
}

static void change_directory(AppState *state, const char *new_dir) {
    char prev_name[256] = "";

    if (strcmp(new_dir, "..") == 0) {
        char temp[MAX_PATH_LEN];
        safe_copy_str(temp, state->current_dir, sizeof(temp));
        char *bname = basename(temp);
        if (bname) safe_copy_str(prev_name, bname, sizeof(prev_name));

        if (chdir("..") == 0) {
            if (getcwd(state->current_dir, sizeof(state->current_dir)) == NULL) {
                safe_copy_str(state->current_dir, "/", sizeof(state->current_dir));
            }
        }
    } else {
        if (chdir(new_dir) == 0) {
            if (getcwd(state->current_dir, sizeof(state->current_dir)) == NULL) {
                safe_copy_str(state->current_dir, new_dir, sizeof(state->current_dir));
            }
        }
    }

    state->filter[0] = '\0';
    reload_directory(state, prev_name);
}

static void do_paste(AppState *state) {
    if (state->clip_op == CLIP_NONE || strlen(state->clip_path) == 0) {
        set_status(state, "Bộ nhớ đệm rỗng! Hãy chọn tệp rồi nhấn 'c' (copy) hoặc 'x' (cut).", 2);
        return;
    }

    struct stat st;
    if (stat(state->clip_path, &st) != 0) {
        set_status(state, "Tệp nguồn trong bộ nhớ đệm không còn tồn tại!", 2);
        state->clip_op = CLIP_NONE;
        return;
    }

    char target_path[MAX_PATH_LEN * 2];
    snprintf(target_path, sizeof(target_path), "%s/%s", state->current_dir, state->clip_name);

    if (stat(target_path, &st) == 0) {
        if (state->clip_op == CLIP_COPY && strcmp(state->clip_path, target_path) == 0) {
            snprintf(target_path, sizeof(target_path), "%s/copy_%s", state->current_dir, state->clip_name);
        } else {
            char confirm_msg[300];
            snprintf(confirm_msg, sizeof(confirm_msg), "Mục '%s' đã tồn tại. Ghi đè?", state->clip_name);
            if (!tui_confirm_dialog("XÁC NHẬN GHI ĐÈ", confirm_msg)) {
                set_status(state, "Đã hủy thao tác dán.", 0);
                return;
            }
        }
    }

    if (state->clip_op == CLIP_COPY) {
        if (file_copy(state->clip_path, target_path) == 0) {
            set_status(state, "Đã sao chép thành công!", 1);
        } else {
            set_status(state, "Lỗi khi sao chép tệp!", 2);
        }
    } else if (state->clip_op == CLIP_CUT) {
        if (file_move(state->clip_path, target_path) == 0) {
            set_status(state, "Đã di chuyển thành công!", 1);
            state->clip_op = CLIP_NONE;
            state->clip_path[0] = '\0';
            state->clip_name[0] = '\0';
        } else {
            set_status(state, "Lỗi khi di chuyển tệp!", 2);
        }
    }

    reload_directory(state, basename(target_path));
}

int main(void) {
    AppState state;
    memset(&state, 0, sizeof(state));

    if (getcwd(state.current_dir, sizeof(state.current_dir)) == NULL) {
        safe_copy_str(state.current_dir, ".", sizeof(state.current_dir));
    }

    state.show_hidden = 0;
    state.sort_mode = SORT_NAME_ASC;

    file_list_init(&state.file_list);
    reload_directory(&state, NULL);

    tui_init();
    tui_clear();

    set_status(&state, "Chào mừng đến với Trình quản lý tệp! Nhấn '?' hoặc 'h' để xem trợ giúp.", 0);

    int running = 1;

    while (running) {
        int rows, cols;
        tui_get_size(&rows, &cols);

        int left_w = (cols * 52) / 100;
        if (left_w < 35) left_w = 35;
        if (left_w > cols - 25) left_w = cols - 25;

        // Vẽ các thành phần giao diện
        render_header(&state, cols);
        render_file_list(&state, rows, left_w);
        render_preview_pane(&state, rows, left_w, cols);
        render_status_bar(&state, rows, cols);
        render_hotkeys_bar(rows, cols);
        tui_flush();

        // Đọc và điều phối phím bấm
        int k = tui_read_key();
        if (k == KEY_NONE) continue;
        if (k == KEY_EOF) break;

        if (k == 'q' || k == 'Q') {
            if (tui_confirm_dialog("THOÁT CHƯƠNG TRÌNH", "Bạn có chắc chắn muốn thoát?")) {
                running = 0;
            }
        } else if (k == KEY_UP) {
            if (state.selected_index > 0) state.selected_index--;
        } else if (k == KEY_DOWN) {
            if (state.selected_index < state.file_list.count - 1) state.selected_index++;
        } else if (k == KEY_PAGE_UP) {
            int page = rows - 6;
            state.selected_index -= page;
            if (state.selected_index < 0) state.selected_index = 0;
        } else if (k == KEY_PAGE_DOWN) {
            int page = rows - 6;
            state.selected_index += page;
            if (state.selected_index >= state.file_list.count)
                state.selected_index = state.file_list.count > 0 ? state.file_list.count - 1 : 0;
        } else if (k == KEY_HOME) {
            state.selected_index = 0;
        } else if (k == KEY_END) {
            state.selected_index = state.file_list.count > 0 ? state.file_list.count - 1 : 0;
        } else if (k == KEY_ENTER || k == KEY_RIGHT) {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (item->is_dir) {
                    change_directory(&state, item->full_path);
                } else {
                    show_info_modal(item);
                }
            }
        } else if (k == KEY_LEFT || k == KEY_BACKSPACE) {
            change_directory(&state, "..");
        } else if (k == ' ') {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (!item->is_dir) {
                    show_pager_modal(item->full_path);
                }
            }
        } else if (k == 'c' || k == 'C') {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (strcmp(item->name, "..") != 0) {
                    safe_copy_str(state.clip_path, item->full_path, sizeof(state.clip_path));
                    safe_copy_str(state.clip_name, item->name, sizeof(state.clip_name));
                    state.clip_op = CLIP_COPY;
                    char msg[300];
                    snprintf(msg, sizeof(msg), "Đã sao chép '%s' vào bộ nhớ đệm.", item->name);
                    set_status(&state, msg, 1);
                }
            }
        } else if (k == 'x' || k == 'X') {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (strcmp(item->name, "..") != 0) {
                    safe_copy_str(state.clip_path, item->full_path, sizeof(state.clip_path));
                    safe_copy_str(state.clip_name, item->name, sizeof(state.clip_name));
                    state.clip_op = CLIP_CUT;
                    char msg[300];
                    snprintf(msg, sizeof(msg), "Đã cắt '%s' vào bộ nhớ đệm.", item->name);
                    set_status(&state, msg, 0);
                }
            }
        } else if (k == 'v' || k == 'V') {
            do_paste(&state);
        } else if (k == 'r' || k == 'R') {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (strcmp(item->name, "..") != 0) {
                    char new_name[256];
                    safe_copy_str(new_name, item->name, sizeof(new_name));
                    if (tui_input_dialog("ĐỔI TÊN", "Nhập tên mới:", new_name, sizeof(new_name))) {
                        if (strlen(new_name) > 0 && strcmp(new_name, item->name) != 0) {
                            char new_path[MAX_PATH_LEN * 2];
                            snprintf(new_path, sizeof(new_path), "%s/%s", state.current_dir, new_name);
                            if (file_move(item->full_path, new_path) == 0) {
                                set_status(&state, "Đổi tên thành công!", 1);
                                reload_directory(&state, new_name);
                            } else {
                                set_status(&state, "Lỗi khi đổi tên tệp!", 2);
                            }
                        }
                    }
                }
            }
        } else if (k == 'd' || k == 'D' || k == KEY_DELETE) {
            if (state.file_list.count > 0) {
                FileItem *item = &state.file_list.items[state.selected_index];
                if (strcmp(item->name, "..") != 0) {
                    char confirm_msg[300];
                    snprintf(confirm_msg, sizeof(confirm_msg), "Xác nhận xóa '%s' vĩnh viễn?", item->name);
                    if (tui_confirm_dialog("XÁC NHẬN XÓA", confirm_msg)) {
                        if (file_delete_recursive(item->full_path) == 0) {
                            set_status(&state, "Đã xóa thành công!", 1);
                            reload_directory(&state, NULL);
                        } else {
                            set_status(&state, "Lỗi khi xóa mục!", 2);
                        }
                    }
                }
            }
        } else if (k == 'n' || k == 'N') {
            char new_file[256] = "";
            if (tui_input_dialog("TẠO TỆP MỚI", "Nhập tên tệp cần tạo:", new_file, sizeof(new_file))) {
                if (strlen(new_file) > 0) {
                    char full[MAX_PATH_LEN * 2];
                    snprintf(full, sizeof(full), "%s/%s", state.current_dir, new_file);
                    if (file_touch(full) == 0) {
                        set_status(&state, "Tạo tệp mới thành công!", 1);
                        reload_directory(&state, new_file);
                    } else {
                        set_status(&state, "Lỗi khi tạo tệp!", 2);
                    }
                }
            }
        } else if (k == 'm' || k == 'M') {
            char new_dir[256] = "";
            if (tui_input_dialog("TẠO THƯ MỤC MỚI", "Nhập tên thư mục mới:", new_dir, sizeof(new_dir))) {
                if (strlen(new_dir) > 0) {
                    char full[MAX_PATH_LEN * 2];
                    snprintf(full, sizeof(full), "%s/%s", state.current_dir, new_dir);
                    if (file_mkdir(full) == 0) {
                        set_status(&state, "Tạo thư mục mới thành công!", 1);
                        reload_directory(&state, new_dir);
                    } else {
                        set_status(&state, "Lỗi khi tạo thư mục!", 2);
                    }
                }
            }
        } else if (k == '/') {
            char filter_buf[128];
            safe_copy_str(filter_buf, state.filter, sizeof(filter_buf));
            if (tui_input_dialog("LỌC TỆP TRỰC TIẾP", "Từ khóa lọc (để trống để hủy lọc):", filter_buf, sizeof(filter_buf))) {
                safe_copy_str(state.filter, filter_buf, sizeof(state.filter));
                reload_directory(&state, NULL);
            }
        } else if (k == KEY_ESC) {
            if (strlen(state.filter) > 0) {
                state.filter[0] = '\0';
                reload_directory(&state, NULL);
                set_status(&state, "Đã hủy bộ lọc.", 0);
            }
        } else if (k == 'o' || k == 'O') {
            state.sort_mode = (state.sort_mode + 1) % 6;
            reload_directory(&state, NULL);
            char sort_msg[128];
            snprintf(sort_msg, sizeof(sort_msg), "Chế độ sắp xếp: %s", sort_mode_name(state.sort_mode));
            set_status(&state, sort_msg, 0);
        } else if (k == '.') {
            state.show_hidden = !state.show_hidden;
            reload_directory(&state, NULL);
            set_status(&state, state.show_hidden ? "Đã bật hiển thị tệp ẩn." : "Đã ẩn tệp ẩn.", 0);
        } else if (k == 's' || k == 'S') {
            char chosen_path[MAX_PATH_LEN] = "";
            int chosen_is_dir = 0;
            if (show_search_modal(state.current_dir, chosen_path, &chosen_is_dir)) {
                if (chosen_is_dir) {
                    change_directory(&state, chosen_path);
                } else {
                    char dir_copy[MAX_PATH_LEN];
                    char file_copy[MAX_PATH_LEN];
                    safe_copy_str(dir_copy, chosen_path, sizeof(dir_copy));
                    safe_copy_str(file_copy, chosen_path, sizeof(file_copy));

                    char *dname = dirname(dir_copy);
                    char *fname = basename(file_copy);

                    if (dname) change_directory(&state, dname);
                    if (fname) reload_directory(&state, fname);
                }
            }
        } else if (k == 't' || k == 'T') {
            show_tree_modal(state.current_dir);
        } else if (k == 'i' || k == 'I') {
            if (state.file_list.count > 0) {
                show_info_modal(&state.file_list.items[state.selected_index]);
            }
        } else if (k == '?' || k == 'h' || k == 'H') {
            show_help_modal();
        }
    }

    file_list_free(&state.file_list);
    tui_cleanup();
    printf("\nCảm ơn bạn đã sử dụng Trình quản lý tệp!\n");
    return 0;
}
