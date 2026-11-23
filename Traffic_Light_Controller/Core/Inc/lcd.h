/*
  lcd.h - Driver màn hình LCD 16x2 (chip HD44780) giao tiếp I2C qua module PCF8574
 
  CÁCH DÙNG:
    1. Trong main(), SAU khi MX_I2C1_Init() và TRƯỚC khi chạy scheduler:
           lcd_init(&hi2c1);
    2. Sau đó dùng các hàm bên dưới trong task của máy trạng thái:
           lcd_clear();
           lcd_set_cursor(0, 0);            // dòng 0, cột 0
           lcd_print_string("Nhiet do: ");
           lcd_print_x10(dht20_get_temperature_x10());   // in dạng 25.3
 
 */
#ifndef LCD_H
#define LCD_H

#include "main.h"
#include <stdint.h>

//config module

#define LCD_I2C_ADDR     0x21U //địa chỉ I2C 7-bit của module LCD

/*
  Cách nối chân của PCF8574 tới LCD (bit nào của byte gửi đi ứng với chân nào).
  Đây là cách nối phổ biến nhất: P0=RS, P1=RW, P2=EN, P3=đèn nền, P4..P7=D4..D7.
  Chân RW luôn bằng 0 (chỉ ghi, không đọc LCD) nên không cần định nghĩa.
 */
#define LCD_PIN_RS       0x01U    // P0: 0 = gửi lệnh, 1 = gửi ký tự  
                                  // P1: RW luôn bằng 0 do chỉ ghi, không đọc              
#define LCD_PIN_EN       0x04U    // P2: xung chốt dữ liệu vào LCD                  
#define LCD_PIN_BL       0x08U    // P3: đèn nền (luôn bật)                         
#define LCD_DATA_SHIFT   4U       // P4 -> P7 mang 4 bit dữ liệu: dịch nửa byte sang trái 4 bit 


// Khởi tạo LCD một lần lúc khởi động, trả về 1 nếu LCD phản hồi, 0 nếu lỗi I2C
uint8_t lcd_init(I2C_HandleTypeDef *hi2c);

// xóa toàn bộ màn hình và đưa con trỏ về đầu dòng 0 (mất khoảng 2 ms, chỉ gọi khi cần)
void    lcd_clear(void);

// đặt vị trí con trỏ: row = 0 hoặc 1, col = 0 -> 15, ký tự in tiếp theo hiện ở vị trí này
void    lcd_set_cursor(uint8_t row, uint8_t col);

// in một chuỗi ký tự (kết thúc bằng '\0') từ vị trí con trỏ hiện tại
void    lcd_print_string(const char *str);

// in một số nguyên có dấu ở dạng thập phân, ví dụ -123
void    lcd_print_int(int32_t value);

// in số theo đơn vị 0.1 với một chữ số thập phân, ví dụ 253 -> "25.3", -5 -> "-0.5"
void    lcd_print_x10(int16_t value);

#endif /* LCD_H */