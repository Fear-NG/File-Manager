# BÁO CÁO ĐỒ ÁN: TRÌNH QUẢN LÝ TỆP TRỰC QUAN (TUI FILE MANAGER)
> **Ngôn ngữ thực hiện:** C (C99 / C11, Tiêu chuẩn POSIX Linux)  
> **Không sử dụng thư viện đồ họa bên ngoài:** Hoàn toàn xây dựng dựa trên các lời gọi hệ thống (System Calls) và mã thoát ANSI (ANSI Escape Sequences).  
> **Nhóm tác giả (Authors):**
> - Nhân Nguyễn (Nguyễn Thành Nhân)
> - Việt Thái
> - Anh Tuấn
> - Huyền Trâm

---

## 1. Giới thiệu & Mục tiêu đề tài
Đồ án nâng cấp ứng dụng quản lý tệp trên hệ điều hành Linux từ giao diện dòng lệnh dạng thực đơn số (CLI Menu) truyền thống sang **giao diện điều khiển trực quan 2 cột (Dual-Pane Terminal User Interface - TUI)** tương tự các công cụ nổi tiếng trên Linux như *Midnight Commander*, *Ranger*.

### Bảng so sánh trước & sau cải tiến:

| Tiêu chí | Phiên bản cũ (CLI) | Phiên bản mới (TUI Cải tiến) |
| :--- | :--- | :--- |
| **Giao diện** | Menu số 1-7 in dòng đơn giản | Giao diện 2 cột toàn màn hình, đóng khung UTF-8 tinh tế |
| **Trải nghiệm duyệt tệp** | Nhập số thứ tự từ bàn phím | Phím mũi tên `↑` `↓` `←` `→`, cuộn trang mượt mà |
| **Xem trước (Preview)** | Phải chọn menu và xem từng tệp | **Live Preview tức thì** khi di chuyển qua từng tệp/thư mục |
| **Hỗ trợ định dạng** | Đọc 15 dòng đầu | Đọc số dòng, trích xuất PDF, DOCX, **Hex Dump cho binary** |
| **Đọc tệp lớn** | Bị cắt ngắn hoặc in tràn terminal | Chế độ **Trình đọc toàn màn hình (Pager)** khi bấm `Space` |
| **Quản lý bộ nhớ** | Mảng tĩnh giới hạn 100 tệp | Mảng động (`realloc`), duyệt thư mục hàng nghìn tệp |
| **Điều hướng** | Gò bó trong thư mục chỉ định | Duyệt tự do vào/ra bất kỳ thư mục nào trên hệ thống |
| **Bộ nhớ đệm** | Không có | Hỗ trợ đầy đủ **Copy (`c`) / Cut (`x`) / Paste (`v`)** |
| **Tìm kiếm** | In danh sách thô | Tìm kiếm đệ quy có hộp thoại cuộn và **nhảy tới tệp** |
| **Sắp xếp & Lọc** | Không hỗ trợ | Lọc nhanh (`/`), đổi sắp xếp (`o`), bật/tắt tệp ẩn (`.`) |
| **Cấu trúc mã nguồn** | Đơn khối (Monolithic 1 file) | **Kiến trúc phân tầng 9 module rõ ràng, độc lập** |

---

## 2. Kiến trúc phân chia module mã nguồn

Toàn bộ hệ thống được phân rã thành các tệp chuyên biệt, hỗ trợ dễ dàng thuyết minh từng slide trong buổi báo cáo:

```
Midterm/
├── file_types.h        # Chứa cấu trúc dữ liệu chung (FileItem, FileList, AppState, Enums)
│
├── tui_core.h/.c       # Hạ tầng điều khiển Terminal: termios raw mode, màu ANSI, khung UTF-8, đọc phím
├── tui_dialogs.h/.c    # Các hộp thoại tương tác: Input Dialog, Confirm Dialog, Alert Dialog
│
├── file_list.h/.c      # Nạp danh sách tệp động, lọc từ khóa, tệp ẩn, thuật toán sắp xếp đa tiêu chí
├── file_crud.h/.c      # Thao tác tệp: Copy, Move, Delete đệ quy, Mkdir, Touch, định dạng và đo ổ đĩa
├── file_preview.h/.c   # Trích xuất xem trước đa định dạng (Text số dòng, PDF, DOCX, Hex binary)
├── file_search.h/.c    # Thu thập cây thư mục (Tree) và công cụ tìm kiếm đệ quy
│
├── ui_views.h/.c       # Vẽ các thành phần giao diện chính: Header, Cột tệp, Cột preview, Status, Hotkeys
├── modals.h/.c         # Cửa sổ pop-up toàn màn hình: Bảng Help, Info, Tree view, Search jump, Pager
│
├── main.c              # Tầng điều phối chính: Vòng lặp sự kiện (Event Loop) và điều phối phím bấm
└── Makefile            # Kịch bản biên dịch toàn bộ dự án
```

---

## 3. Các lời gọi hệ thống Linux (System Calls & POSIX APIs) được ứng dụng

Điểm nhấn quan trọng trong đồ án Hệ Điều Hành / Lập trình Hệ Thống:

1. **`termios` (`tcgetattr`, `tcsetattr`)**:
   - Vô hiệu hóa chế độ `ICANON` và `ECHO` để chuyển terminal sang **Raw Mode**.
   - Giúp ứng dụng nhận từng phím bấm ngay lập tức mà không cần người dùng nhấn phím Enter.
2. **`ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)`**:
   - Lấy kích thước số dòng (`ws_row`) và số cột (`ws_col`) của terminal để tự động căn chỉnh giao diện co giãn linh hoạt (Responsive layout).
3. **`statvfs()`**:
   - Truy vấn thông số hệ thống tệp để tính toán chính xác dung lượng còn trống (`f_bavail * f_frsize`) và hiển thị trên thanh tiêu đề.
4. **`opendir()`, `readdir()`, `closedir()`**:
   - Mở và duyệt các mục trong thư mục Linux mà không phụ thuộc vào lệnh ngoài.
5. **`lstat()` & `stat()`**:
   - Đọc thông tin chi tiết của từng tệp: kích thước (`st_size`), quyền truy cập (`st_mode`), thời gian chỉnh sửa (`st_mtime`), phân biệt thư mục (`S_ISDIR`), liên kết mềm (`S_ISLNK`).
6. **`mkdir()`, `rmdir()`, `unlink()`, `rename()`**:
   - Thực thi các thao tác tạo thư mục, xóa tệp, đổi tên và di chuyển tệp.
7. **`popen()`, `pclose()`**:
   - Mở luồng pipe để giao tiếp an toàn với các công cụ tiện ích của Linux (`pdfinfo`, `pdftotext`, `exiftool`, `docx2txt`).
8. **Mã thoát ANSI (ANSI Escape Sequences)**:
   - `\033[?1049h` / `\033[?1049l`: Chuyển đổi qua lại giữa màn hình chính và Alternate Screen Buffer.
   - `\033[?25l` / `\033[?25h`: Ẩn / hiện con trỏ chuột.
   - `\033[row;colH`: Di chuyển con trỏ vẽ giao diện tại tọa độ bất kỳ.

---

## 4. Hướng dẫn biên dịch & Thực thi

### Yêu cầu môi trường:
- Hệ điều hành: Linux (Ubuntu, Debian, Fedora, Arch Linux, WSL...)
- Trình biên dịch: `gcc` và công cụ `make`
- *(Tùy chọn cho xem trước file nâng cao)*: `poppler-utils` (cho PDF), `docx2txt` / `libimage-exiftool-perl` (cho DOCX).

### Các lệnh biên dịch:
```bash
# Biên dịch toàn bộ đồ án:
make

# Dọn dẹp các tệp object:
make clean

# Chạy ứng dụng:
./file_manager
```

---

## 5. Bảng tra cứu phím tắt điều khiển

| Nhóm chức năng | Phím | Tác vụ |
| :--- | :---: | :--- |
| **Điều hướng** | `↑` / `↓` | Di chuyển lên / xuống danh sách tệp |
| | `PgUp` / `PgDn` | Cuộn nhanh một trang màn hình |
| | `Home` / `End` | Nhảy về đầu danh sách / cuối danh sách |
| | `Enter` / `→` | Mở thư mục / Xem thông tin chi tiết tệp |
| | `←` / `Backspace` | Lùi về thư mục cha (`..`) |
| **Xem nội dung** | `Space` | **Mở trình đọc tệp toàn màn hình (File Pager)** |
| **Clipboard** | `c` | Sao chép (Copy) tệp hoặc thư mục |
| | `x` | Cắt (Cut) tệp hoặc thư mục |
| | `v` | Dán (Paste) vào thư mục hiện tại |
| **Thao tác tệp** | `r` | Đổi tên tệp / thư mục (Rename) |
| | `d` / `Delete` | Xóa tệp / thư mục (Hộp thoại xác nhận an toàn) |
| | `n` | Tạo tệp tin rỗng mới (New File) |
| | `m` | Tạo thư mục mới (New Directory) |
| **Tìm kiếm & Lọc** | `/` | Lọc tức thì tệp trong thư mục hiện tại (`Esc` để hủy) |
| | `s` | **Tìm kiếm đệ quy toàn hệ thống & Nhảy tới tệp** |
| | `t` | Hiển thị sơ đồ cây thư mục (Directory Tree) |
| | `o` | Đổi chế độ sắp xếp (Tên A-Z, Z-A, Dung lượng, Mới nhất) |
| | `.` | Bật / Tắt hiển thị tệp ẩn (`.dotfiles`) |
| **Hệ thống** | `i` | Xem chi tiết siêu dữ liệu (Metadata / Permissions) |
| | `?` hoặc `h` | Mở bảng hướng dẫn trợ giúp |
| | `q` / `Esc` | Thoát ứng dụng |

---

## 6. Kịch bản đề xuất cho buổi Demo / Thuyết trình

1. **Bước 1 (Giới thiệu giao diện):**
   - Khởi chạy `./file_manager`.
   - Giới thiệu bố cục 2 cột: Cột trái danh sách tệp, Cột phải live preview.
   - Chỉ ra thanh tiêu đề hiển thị **dung lượng ổ đĩa trống** (`statvfs`) và thanh phím tắt dưới cùng.

2. **Bước 2 (Duyệt tệp & Live Preview):**
   - Dùng phím `↓` di chuyển qua các tệp mã nguồn C (`main.c`, `file_crud.c`), giải thích cách ứng dụng tự động hiển thị số dòng.
   - Di chuyển vào thư mục `test_files`:
     - Trỏ vào `demo_pdf.pdf` để thấy trích xuất thông tin PDF.
     - Trỏ vào `demo_doc.docx` để thấy siêu dữ liệu DOCX.
     - Trỏ vào file nhị phân `file_manager` để thấy bảng Hex Dump tự động.

3. **Bước 3 (Đọc toàn màn hình - Pager):**
   - Chọn `main.c`, nhấn phím `Space` để mở trình đọc file toàn màn hình, dùng `PgDn` cuộn đọc và nhấn `Space` hoặc `Esc` để đóng.

4. **Bước 4 (Thao tác tệp & Clipboard):**
   - Nhấn `n` tạo tệp `demo.txt`.
   - Chọn `demo.txt`, nhấn `c` (Copy), đổi tên bằng `r`, sau đó nhấn `v` (Paste).
   - Chọn tệp sao chép, nhấn `d` xóa tệp và nhấn `y` trong hộp thoại xác nhận.

5. **Bước 5 (Tìm kiếm & Cây thư mục):**
   - Nhấn `t` để xem sơ đồ cây thư mục trực quan.
   - Nhấn `s` tìm từ khóa (ví dụ `demo`), danh sách kết quả hiện ra, chọn một kết quả và nhấn `Enter` để ứng dụng tự động chuyển đến đúng vị trí tệp đó!
