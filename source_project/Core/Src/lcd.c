/*
 * lcd.c - Driver LCD 16x2 (HD44780) qua I2C. Xem lcd.h để biết cách dùng và cấu hình.
 *
 * NGUYÊN LÝ:
 *   - LCD HD44780 nhận hai loại byte: LỆNH (chân RS=0, ví dụ xóa màn hình, đặt con trỏ)
 *     và KÝ TỰ (chân RS=1, mã ASCII cần hiển thị).
 *   - LCD chốt dữ liệu vào ở cạnh XUỐNG của chân EN: đặt dữ liệu, kéo EN lên 1, rồi kéo về 0.
 *   - Module chỉ nối 4 chân dữ liệu (D4..D7) nên mỗi byte được gửi làm hai lần,
 *     4 bit cao trước rồi 4 bit thấp (gọi là chế độ 4 bit).
 *   - Các chân RS, EN, đèn nền và D4..D7 của LCD được nối vào 8 chân của chip PCF8574.
 *     Chip này là bộ mở rộng I2C: byte ghi vào địa chỉ của nó chính là mức logic
 *     của 8 chân. Vì vậy điều khiển LCD = ghi các byte vào PCF8574 qua I2C.
 *   - Một lần truyền I2C có thể ghi nhiều byte liên tiếp, nên mỗi ký tự (4 byte:
 *     EN=1 rồi EN=0 cho nửa cao, tiếp theo cho nửa thấp) chỉ mất một lần truyền.
 */
#include "lcd.h"

// thời gian chờ tối đa cho một lần truyền I2C (ms), để không treo nếu LCD không trả lời
#define LCD_I2C_TIMEOUT_MS   10U

// Các lệnh của HD44780 (theo datasheet)
#define LCD_CMD_CLEAR        0x01U   // xóa màn hình, con trỏ về đầu dòng 0                  
#define LCD_CMD_ENTRY_MODE   0x06U   // con trỏ tự dịch sang phải sau mỗi ký tự, không cuộn màn hình
#define LCD_CMD_DISPLAY_OFF  0x08U   // tắt hiển thị (dữ liệu trong LCD vẫn còn) 
#define LCD_CMD_DISPLAY_ON   0x0CU   // bật hiển thị, ẩn con trỏ, không nhấp nháy
#define LCD_CMD_FUNCTION_SET 0x28U   // giao tiếp 4 bit, 2 dòng, font 5x8
#define LCD_CMD_SET_DDRAM    0x80U   // đặt địa chỉ con trỏ: 0x80 | địa chỉ
#define LCD_ROW1_OFFSET      0x40U   // dòng 1 của LCD 16x2 bắt đầu ở địa chỉ DDRAM 0x40 chứ không phải 0x10
#define LCD_COLS             16U     // số cột của LCD

//biến nội bộ

// Con trỏ tới handle I2C (hi2c1), được lưu lại lúc lcd_init()
static I2C_HandleTypeDef *lcd_i2c;

//hàm nội bộ
/*
  lcd_write_nibble - Gửi MỘT nửa byte (4 bit) tới LCD, dùng riêng cho lúc khởi tạo.
    nibble: 4 bit cần gửi (0x0..0xF)
    Trả về: 1 nếu truyền I2C thành công, 0 nếu lỗi.
  Gửi hai byte tới PCF8574: byte đầu có EN=1, byte sau có EN=0 (cạnh xuống chốt dữ liệu).
  Chân RS luôn bằng 0 vì lúc khởi tạo chỉ gửi lệnh.
*/
static uint8_t lcd_write_nibble(uint8_t nibble)
{
    uint8_t data = (uint8_t)((nibble & 0x0FU) << LCD_DATA_SHIFT);  // đưa 4 bit vào đúng chân D4 -> D7
    uint8_t buf[2];

    buf[0] = (uint8_t)(data | LCD_PIN_BL | LCD_PIN_EN);   // EN = 1 
    buf[1] = (uint8_t)(data | LCD_PIN_BL);                // EN = 0: cạnh xuống chốt dữ liệu

    return (HAL_I2C_Master_Transmit(lcd_i2c, (uint16_t)(LCD_I2C_ADDR << 1), buf, 2, LCD_I2C_TIMEOUT_MS) == HAL_OK) ? 1U : 0U;
}

/*
  lcd_write_byte - Gửi MỘT byte đầy đủ (hai nửa byte) tới LCD.
    value: byte cần gửi (mã lệnh hoặc mã ký tự)
    rs   : 0 nếu là lệnh, LCD_PIN_RS nếu là ký tự
  Dùng một lần truyền I2C gồm 4 byte: nửa cao (EN=1, EN=0) rồi nửa thấp (EN=1, EN=0).
  Lỗi I2C bị bỏ qua (ký tự đó không hiện), việc phát hiện LCD có mặt chỉ làm trong lcd_init().
 */
//hàm gửi lệnh hoặc ký tự
static void lcd_write_byte(uint8_t value, uint8_t rs)
{
    uint8_t ctrl = (uint8_t)(LCD_PIN_BL | rs);                          // đèn nền + RS, dùng cho cả 4 byte
    //điền 4 bit cao xong tới 4 bit thấp vào chân D4->D7
    uint8_t hi   = (uint8_t)((value >> 4) << LCD_DATA_SHIFT);           // nửa cao đặt vào chân dữ liệu
    uint8_t lo   = (uint8_t)((value & 0x0FU) << LCD_DATA_SHIFT);        // nửa thấp đặt vào chân dữ liệu 
    uint8_t buf[4];
    
    buf[0] = (uint8_t)(hi | ctrl | LCD_PIN_EN);   // nửa cao, EN = 1
    buf[1] = (uint8_t)(hi | ctrl);                // nửa cao, EN = 0: LCD chốt nửa cao 
    buf[2] = (uint8_t)(lo | ctrl | LCD_PIN_EN);   // nửa thấp, EN = 1
    buf[3] = (uint8_t)(lo | ctrl);                // nửa thấp, EN = 0: LCD chốt nửa thấp

    (void)HAL_I2C_Master_Transmit(lcd_i2c, (uint16_t)(LCD_I2C_ADDR << 1), buf, 4, LCD_I2C_TIMEOUT_MS);
}

// hàm gửi lệnh(từng byte), RS=0 tới LCD
static void lcd_send_command(uint8_t cmd)
{
    lcd_write_byte(cmd, 0U);
}

//hàm gửi data(từng byte), RS=1 tới LCD, ký tự sẽ hiện ở vị trí con trỏ
static void lcd_send_data(uint8_t ch)
{
    lcd_write_byte(ch, LCD_PIN_RS);
}

//hàm global


//Trình tự theo datasheet HD44780 cho chế độ 4 bit:
//Chờ 50 ms (datasheet yêu cầu ít nhất 40 ms để nguồn LCD ổn định).
//Gửi 0x3, chờ 5 ms, gửi 0x3, chờ 1 ms, gửi 0x3, chờ 1 ms. Gửi ba lần để đưa LCD về trạng thái đã biết dù trước đó nó đang ở trạng thái nào (ví dụ MCU vừa reset nhưng LCD vẫn có điện).
//Gửi 0x2 để chuyển sang chế độ 4 bit.
//Gửi lần lượt 0x28, 0x08, xóa màn hình, 0x06, 0x0C.
//Lần ghi đầu tiên cũng là phép kiểm tra sự có mặt: nếu không có thiết bị trả lời ACK thì HAL báo lỗi và lcd_init trả 0.
uint8_t lcd_init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c = hi2c;

    HAL_Delay(50);                       // chờ nguồn LCD ổn định (datasheet: >= 40 ms)

    if (!lcd_write_nibble(0x3U)) {       // lần 1: đồng thời kiểm tra LCD có phản hồi trên I2C không
        return 0;
    }
    HAL_Delay(5);                        // datasheet: chờ > 4.1 ms
    (void)lcd_write_nibble(0x3U);        // lần 2
    HAL_Delay(1);                        // datasheet: chờ > 100 us 
    (void)lcd_write_nibble(0x3U);        // lần 3
    HAL_Delay(1);
    (void)lcd_write_nibble(0x2U);        // chuyển sang giao tiếp 4 bit
    HAL_Delay(1);

    lcd_send_command(LCD_CMD_FUNCTION_SET);   // 4 bit, 2 dòng, font 5x8
    lcd_send_command(LCD_CMD_DISPLAY_OFF);    // tắt hiển thị trong lúc cấu hình
    lcd_clear();                              // xóa màn hình
    lcd_send_command(LCD_CMD_ENTRY_MODE);     // con trỏ tự dịch phải sau mỗi ký tự
    lcd_send_command(LCD_CMD_DISPLAY_ON);     // bật hiển thị, ẩn con trỏ

    return 1;
}


//hàm xoá toàn màn hình, con trỏ về đầu dòng 0
//Gửi 0x01 rồi chờ 2 ms, vì lệnh này LCD xử lý chậm (khoảng 1,52 ms)
//Vì có HAL_Delay nên không gọi trong task, khi cập nhật số -> ghi đè bằng khoảng trắng.
void lcd_clear(void)
{
    lcd_send_command(LCD_CMD_CLEAR);
    HAL_Delay(2);                        // chờ LCD xóa xong (datasheet: 1.52 ms)
}

//Địa chỉ DDRAM của HD44780 không liên tục: dòng 0 bắt đầu ở 0x00, dòng 1 ở 0x40
//Hàm cộng địa chỉ đầu dòng với cột rồi gửi lệnh 0x80 | địa chỉ. Cột bị giới hạn 
//ở 15 để tránh ghi ra ngoài dòng.
//đặt con trỏ trước rồi khi print sẽ print tại vị trí con trỏ đang trỏ tới
void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0U) ? 0x00U : LCD_ROW1_OFFSET;   // địa chỉ đầu dòng

    if (col >= LCD_COLS) {
        col = (uint8_t)(LCD_COLS - 1U);                      // giới hạn trong 16 cột
    }
    lcd_send_command((uint8_t)(LCD_CMD_SET_DDRAM | (addr + col))); //lệnh đặt địa chỉ DDRAM: bit 7 = 1, 7 bit còn lại là địa chỉ
}

/*
  lcd_print_string - In chuỗi ký tự từ vị trí con trỏ hiện tại.
    str: chuỗi kết thúc bằng '\0' (ví dụ "Nhiet do: "). LCD chỉ có chữ không dấu.
 */
//hàm in chuỗi
void lcd_print_string(const char *str)
{
    //gửi từng ký tự đến khi gặp ký tự kết thúc chuỗi
    while (*str != '\0') {
        lcd_send_data((uint8_t)*str);
        str++;
    }
}

/*
 lcd_print_int - In một số nguyên có dấu ở dạng thập phân.
    value: số cần in, ví dụ -123 hiện "-123"
    Cách làm: tách chữ số từ hàng đơn vị lên (lấy phần dư khi chia 10) vào mảng,
    rồi in ngược lại từ chữ số cao nhất. Dùng số không dấu 32 bit để xử lý đúng cả
    giá trị âm nhỏ nhất.
*/
//hàm in số nguyên có dấu
void lcd_print_int(int32_t value)
{
    char digits[10];                 // số 32 bit có tối đa 10 chữ số
    uint8_t count = 0;
    uint32_t u;                          // độ lớn của số (không dấu)

    if (value < 0) {
        lcd_send_data('-');              // in dấu trừ trước
        u = (uint32_t)(-(value + 1)) + 1U;   //lấy giá trị tuyệt đối, tránh tràn với số âm nhỏ nhất
    } else {
        u = (uint32_t)value;
    }
    //lấy từng ký tự từ phải sang trái, rồi in ra hướng ngược lại
    do {
        digits[count] = (char)('0' + (u % 10U));   //chữ số hàng đơn vị đổi sang mã ASCII
        count++;
        u /= 10U;                                  // bỏ chữ số vừa lấy
    } while (u > 0U);

    while (count > 0U) {                 // in ngược
        count--;
        lcd_send_data((uint8_t)digits[count]);
    }
}

/*
  lcd_print_x10 - In một số theo đơn vị 0.1 với đúng một chữ số thập phân.
    value: ví dụ 253 -> "25.3", -5 -> "-0.5", 1000 -> "100.0"
  Dùng cho giá trị của dht20_get_temperature_x10() và dht20_get_humidity_x10().
  Phải in dấu trừ riêng, vì nếu chỉ in value / 10 thì số -0.5 sẽ mất dấu (-5 / 10 = 0).
 */
//hàm in số theo đơn vị 0,1
void lcd_print_x10(int16_t value)
{
    uint16_t u;                          // độ lớn của số (không dấu)

    if (value < 0) {
        lcd_send_data('-');              // in dấu trừ riêng 
        u = (uint16_t)(-(int32_t)value);
    } else {
        u = (uint16_t)value;
    }

    lcd_print_int((int32_t)(u / 10U));   // phần nguyên
    lcd_send_data('.');
    lcd_send_data((uint8_t)('0' + (u % 10U)));   // một chữ số thập phân
}