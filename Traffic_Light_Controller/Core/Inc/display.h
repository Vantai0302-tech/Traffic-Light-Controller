/*
  display.h - Hiển thị lên LCD: tên chế độ, nhiệt độ và độ ẩm (không chứa lệnh HAL, chỉ gọi hàm lcd_*)
 
  CÁCH DÙNG:
    1. Đăng ký một task chu kỳ 1 giây vào scheduler (tick 10 ms nên là 100 tick):
           SCH_Add_Task(display_run, 0, 100);
    2. fsm_mode gọi display_refresh() ngay khi đổi chế độ để LCD đổi liền, không chờ 1 giây.
 
  BỐ CỤC (LCD 16 cột, chế độ AUTO và MANUAL):
       Mode: AUTO
       T:25.3C H:60.4%
  Ở chế độ TUNING, fsm_tuning tự vẽ màn hình của nó nên display bỏ qua.
 
  Không dùng lcd_clear() vì hàm đó có HAL_Delay, không được gọi trong task.
  Mỗi dòng được ghi đủ 16 cột (thêm khoảng trắng) để ghi đè hết chữ cũ.
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

#define DISPLAY_LINE_WIDTH  16U     // số cột của LCD

// in một dòng chữ từ cột 0 của hàng row, tự thêm khoảng trắng cho đủ 16 cột để xóa chữ cũ
void display_print_line(uint8_t row, const char *text);

// vẽ lại toàn bộ màn hình ở chế độ AUTO/MANUAL (bỏ qua nếu đang ở MODE_TUNING)
void display_refresh(void);

// task của scheduler, gọi mỗi 1 giây để cập nhật nhiệt độ và độ ẩm
void display_run(void);

#endif /* DISPLAY_H */
