# Traffic-Light-Controller

Hệ thống đèn giao thông 2 tuyến đường trên mạch **XNUCLEO-F103RB** (STM32F103RB), bài tập lớn môn Vi xử lý – Vi điều khiển (CO3009), Trường Đại học Bách khoa – ĐHQG TP.HCM.

Toàn bộ hệ thống chạy trên **scheduler O(1)** (mảng task sắp xếp theo Delay tương đối, hỗ trợ one-shot task) kết hợp các máy trạng thái. Không dùng `HAL_Delay` trong các task.

## Chức năng

- **Chế độ tự động (AUTO):** hai tuyến chạy luân phiên 4 pha, thời gian mặc định Xanh 8 s, Vàng 2 s, Đỏ 10 s (Đỏ = Xanh + Vàng).
- **Chế độ bằng tay (MANUAL):** đèn đứng yên, nhấn nút để sang pha kế tiếp.
- **Chế độ chỉnh thời gian (TUNING):** chọn đèn Đỏ / Xanh / Vàng, tăng giảm thời gian rồi xác nhận; giá trị được kiểm tra ràng buộc (1..99 s, Đỏ = Xanh + Vàng, Vàng < Xanh). Tuyến 1 nháy 2 Hz theo đèn đang chọn, tuyến 2 giữ Đỏ.
- **Hiển thị:** LCD 16x2 hiện tên chế độ, nhiệt độ và độ ẩm đo từ DHT20 (đo mỗi 2 s).
- **Khởi động an toàn:** sau khi cấp nguồn hai tuyến giữ Đỏ 2 s, rồi một one-shot task (`fsm_mode_boot_done`) mới cho chế độ tự động chạy.

## Phần cứng

| Thiết bị | Chân Arduino | Chân STM32 | Ghi chú |
|---|---|---|---|
| Đèn giao thông 1 | D3, D4 | PB3, PB5 | `TL1_A`, `TL1_B` |
| Đèn giao thông 2 | D5, D6 | PB4, PB10 | `TL2_A`, `TL2_B` |
| Nút 1, 2 | D7, D8 | PA8, PA9 | `BTN_1`, `BTN_2` |
| Nút 3, 4 | D9, D10 | PC7, PB6 | `BTN_3`, `BTN_4` |
| Nút USER | – | PC13 | `B1` |
| DHT20, LCD (I2C1) | Khe I2C | SCL PB8, SDA PB9 | DHT20 `0x38`, LCD `0x21` |

- Mã điều khiển mỗi module đèn (tín hiệu 1, tín hiệu 2): Xanh = `10`, Vàng = `01`, Đỏ = `11`, Tắt = `00`.
- Các nút nhấn ở mức 0 khi nhấn (kể cả nút USER).
- Địa chỉ I2C của LCD tùy module PCF8574 (thường `0x27` hoặc `0x21`). Nếu LCD không hiện chữ, quét I2C rồi sửa `LCD_I2C_ADDR` trong `Core/Inc/lcd.h`.

## Cấu trúc thư mục

```
Traffic-Light-Controller/
├── Traffic_Light_Controller/        # dự án STM32CubeIDE
│   ├── Core/
│   │   ├── Inc/   global.h, scheduler.h, software_timer.h, button.h, traffic_light.h,
│   │   │          dht20.h, lcd.h, display.h, fsm_mode.h, fsm_automatic.h,
│   │   │          fsm_manual.h, fsm_tuning.h, main.h, ...
│   │   └── Src/   các file .c tương ứng, main.c, ...
│   ├── Drivers/                     # HAL và CMSIS do STM32CubeMX sinh ra
│   ├── CMakeLists.txt, CMakePresets.json
│   └── Traffic_Light_Controller.ioc
├── .gitignore
└── README.md
```

Các lớp của mã nguồn: `main.c` (khởi tạo, đăng ký task) → máy trạng thái `fsm_*.c` (không có lệnh HAL) → module phụ trợ (`software_timer`, `button`, `traffic_light`, `dht20`, `lcd`, `display`, `global`) → HAL. Tick của scheduler là 10 ms (TIM2: Prescaler 6399, Counter Period 99).

## Build và nạp chương trình

### Cách 1: STM32CubeIDE (khuyến nghị)

1. Clone mã nguồn:
   ```
   git clone https://github.com/Vantai0302-tech/Traffic-Light-Controller.git
   ```
2. Mở STM32CubeIDE, chọn **File → Open Projects from File System**, trỏ tới thư mục `Traffic_Light_Controller` rồi **Finish**.
3. Kiểm tra cấu hình: board NUCLEO-F103RB, **SYS → Debug: Serial Wire**.
4. Build bằng **Ctrl + B**.
5. Cắm mạch bằng cáp USB (ST-LINK trên mạch) và nạp bằng **Run**.

### Cách 2: CMake (tùy chọn, cần `arm-none-eabi-gcc` và Ninja)

```
cd Traffic_Light_Controller
cmake --preset Debug
cmake --build --preset Debug
```

File `.elf` nằm trong `build/Debug/`.

## Hướng dẫn vận hành

Nút 1 (D7) đổi chế độ theo vòng AUTO → MANUAL → TUNING → AUTO.

| Nút | AUTO | MANUAL | TUNING |
|---|---|---|---|
| Nút 1 (D7) | Sang MANUAL | Sang TUNING | Về AUTO (bỏ giá trị chưa xác nhận) |
| Nút 2 (D8) | – | Sang pha kế tiếp | Tăng 1 s (giữ trên 1 s: tăng thêm 10 s) |
| Nút 3 (D9) | – | – | Giảm 1 s (giữ trên 1 s: giảm thêm 10 s) |
| Nút 4 (D10) | – | – | Chọn đèn cần chỉnh: Đỏ → Xanh → Vàng |
| USER | – | – | Xác nhận giá trị |

Sau khi nạp: hai tuyến Đỏ trong 2 s (nút bấm chưa có tác dụng), rồi chạy chế độ tự động. Trong TUNING, LCD hiện `Chinh: <đèn>` và `Gia tri: <n> s`; bấm USER thì LCD báo `Da luu!` nếu hợp lệ hoặc `Gia tri sai!` nếu vi phạm ràng buộc.

Ví dụ đổi thời gian Xanh thành 10 s: nhấn nút 1 hai lần để vào TUNING, nhấn nút 4 một lần để chọn Xanh, nhấn nút 2 hai lần để được 10, nhấn USER. Thời gian Đỏ tự cập nhật thành 12 s. Nhấn nút 1 để về AUTO, thời gian mới có tác dụng từ pha đầu.

Thời gian đã chỉnh chưa được lưu vào Flash, nên reset hoặc cấp nguồn lại sẽ về 10/8/2.

## Quy ước mã nguồn

- Tên file chữ thường nối bằng gạch dưới (`fsm_automatic.c`); hàm dạng `<module>_<hành_động>` (`fsm_manual_init`, `dht20_update`); scheduler dùng tiền tố `SCH_`.
- Máy trạng thái dùng `enum` trạng thái viết hoa và `switch/case` có `default`.
- Quy trình Git: mỗi phần việc một nhánh `feature/...`, gộp vào `dev`, chỉ gộp `dev` vào `main` ở cuối và gắn tag nộp bài `v1.0-final`.
