

### 📂 Cây thư mục `components/sim4g_gps`:

```
sim4g_gps/
├── CMakeLists.txt        # ①
├── include/              # ②
│   └── sim4g_gps.h       # ③
├── Kconfig               # ④
├── README.md             # ⑤
└── src/                  # ⑥
    ├── sim4g_at.c        # ⑦
    ├── sim4g_at_cmd.h    # ⑧
    ├── sim4g_at.h        # ⑨
    └── sim4g_gps.c       # ⑩
```

---

## 🧠 Chú thích chuyên sâu từng mục:

| # | Tên file/thư mục      | Giải thích chi tiết chuyên nghiệp                                                                                                                                     |
| - | --------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ① | `CMakeLists.txt`      | File cấu hình build cho component này. Khai báo các file `.c`, thư mục `include`, dependencies,... để ESP-IDF nhận diện và biên dịch đúng.                            |
| ② | `include/`            | Thư mục chứa **header công khai (public API)** của component. Các component hoặc `main/` khác sẽ `#include "sim4g_gps.h"` từ đây.                                     |
| ③ | `include/sim4g_gps.h` | Header chính để tương tác với toàn bộ component `sim4g_gps`. Đây là **cổng vào chính (facade)**: khai báo các hàm public của `sim4g_gps.c`.                           |
| ④ | `Kconfig`             | Cho phép người dùng bật/tắt cấu hình từ `menuconfig`. Ví dụ: chọn UART nào, enable debug,... Đây là chuẩn công nghiệp cho ESP-IDF component cấu hình được.            |
| ⑤ | `README.md`           | Ghi chú tài liệu nội bộ: hướng dẫn dùng component, API, sơ đồ thiết kế. Dành cho người dùng khác hoặc chính bạn sau này đọc lại.                                      |
| ⑥ | `src/`                | Thư mục chứa toàn bộ mã nguồn `.c` và các header nội bộ (`.h`) **không export ra ngoài**, dùng nội bộ trong component này.                                            |
| ⑦ | `sim4g_at.c`          | Xử lý logic liên quan đến AT command: gửi, nhận, parse kết quả. Thường là các hàm `sim4g_at_send()`, `sim4g_at_expect_ok()`...                                        |
| ⑧ | `sim4g_at_cmd.h`      | File chỉ chứa các chuỗi lệnh AT dưới dạng macro. Dạng như `#define AT_CMD_GPS_START "AT+QGPS=1"`. Đây là **constant data header**, dùng lại ở nhiều nơi.              |
| ⑨ | `sim4g_at.h`          | Khai báo các hàm trong `sim4g_at.c`, tức là **interface nội bộ** cho module xử lý AT command. Có thể được include bởi `sim4g_gps.c` hoặc module khác trong component. |
| ⑩ | `sim4g_gps.c`         | Phần "high-level API": tổ chức gọi chuỗi lệnh AT để lấy vị trí GPS, gửi tin nhắn,... Đây là nơi gọi `sim4g_at_*()` từ `sim4g_at.c`. Public API sẽ nằm ở đây.          |

---

## 🧩 Mối quan hệ giữa các file

```
app/main.c
   │
   └──> #include "sim4g_gps.h"
             │
             └──> sim4g_gps.c
                       └──> #include "sim4g_at.h"
                                  └──> sim4g_at.c
                       └──> #include "sim4g_at_cmd.h"
```

┌────────────────────────┐
│ Component: sim4g_gps   │ ← Public API
└────────────────────────┘
           │
           ▼
┌────────────────────────┐
│ sim4g_at.c             │ ← Xử lý command AT (core logic)
│ - Gửi, nhận, parse     │
│ - Tách biệt giao tiếp  │
└────────────────────────┘
           │
           ▼
┌────────────────────────┐
│ comm.c (UART layer)    │ ← Gửi dữ liệu raw qua UART
└────────────────────────┘
           │
           ▼
   Module SIM EC800K (4G+GPS)
