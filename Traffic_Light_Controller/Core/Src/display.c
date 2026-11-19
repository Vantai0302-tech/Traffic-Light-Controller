/*
 * display.c - Hiển thị chế độ, nhiệt độ, độ ẩm lên LCD. Xem display.h để biết cách dùng.
 */
#include "display.h"
#include "global.h"
#include "lcd.h"
#include "dht20.h"
#include <string.h>

/*
  hàm in một dòng chữ và xóa phần chữ cũ còn thừa phía sau.
    row : 0 (dòng trên) hoặc 1 (dòng dưới)
    text: chuỗi kết thúc bằng '\0', không dấu
  Vì không gọi lcd_clear() (có HAL_Delay) nên phải tự ghi khoảng trắng cho đủ 16 cột,
  nếu không chữ dài của lần trước sẽ còn sót lại (ví dụ "100.0" rồi "99.9" thành "99.90").
 */
void display_print_line(uint8_t row, const char *text)
{
    size_t len = strlen(text);
    //đặt con trỏ đầu dòng, in chuỗi rồi thêm khoảng trắng cho đủ 16 cột
    lcd_set_cursor(row, 0);
    lcd_print_string(text);
    while (len < DISPLAY_LINE_WIDTH) {
        lcd_print_string(" ");
        len++;
    }
}

/*
  hàm vẽ lại màn hình ở chế độ AUTO hoặc MANUAL.
  Dòng 0: tên chế độ. Dòng 1: nhiệt độ và độ ẩm lấy từ dht20.
  Nếu DHT20 chưa có lần đo hợp lệ nào thì hiện "--.-" thay vì số 0 để không gây hiểu nhầm.
  Ở MODE_TUNING hàm không làm gì vì fsm_tuning tự điều khiển LCD.
 */
void display_refresh(void)
{
    //Tuning hiển thị kiểu khác
    if (system_mode == MODE_TUNING) {
        return;
    }

    // dòng 0: tên chế độ
    switch (system_mode) {
    case MODE_AUTO:
        display_print_line(0U, "Mode: AUTO");
        break;
    case MODE_MANUAL:
        display_print_line(0U, "Mode: MANUAL");
        break;
    default:
        break;
    }

    // dòng 1: nhiệt độ và độ ẩm
    lcd_set_cursor(1U, 0U);
    if (dht20_is_valid()) {
        lcd_print_string("T:");
        lcd_print_x10(dht20_get_temperature_x10());       // vd:25.3 thì lấy 253 rồi in ra 25 -> . -> 3
        lcd_print_string("C H:");
        lcd_print_x10((int16_t)dht20_get_humidity_x10());  // tương tự
        lcd_print_string("%  ");                           // 2 khoảng trắng xóa chữ thừa của lần trước
    } else {
        lcd_print_string("T:--.-C H:--.-% ");
    }
}

/*
  display_run - Task của scheduler, gọi mỗi 1 giây để làm mới số liệu.
  DHT20 chỉ đo mỗi 2 giây nên gọi thường xuyên hơn không có ích và tốn thời gian I2C.
 */
void display_run(void)
{
    display_refresh();
}
