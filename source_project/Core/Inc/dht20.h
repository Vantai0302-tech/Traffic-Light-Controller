/*
 * dht20.h - Driver cảm biến nhiệt độ / độ ẩm DHT20 (giao tiếp I2C, địa chỉ 0x38)
 *
 * CÁCH DÙNG:
 *   1. Trong main(), SAU khi MX_I2C1_Init() và TRƯỚC khi chạy scheduler:
 *          dht20_init(&hi2c1);
 *   2. Đăng ký một task chu kỳ 100 ms vào scheduler (tick 10 ms nên là 10 tick):
 *          SCH_Add_Task(dht20_update, 0, 10);
 *      Task này tự đo mỗi 2 giây, không chặn chương trình.
 *   3. Khi cần hiển thị (ví dụ lên LCD), lấy giá trị bằng các hàm get bên dưới.
 *
 * GIÁ TRỊ TRẢ VỀ ở dạng số nguyên theo phần mười, để không phải dùng số thực:
 *      dht20_get_temperature_x10() == 253   ->  25.3 độ C
 *      dht20_get_humidity_x10()    == 604   ->  60.4 %RH
 *
 * theo đề, module này là nơi duy nhất (cùng main.c) được gọi hàm HAL.
 */
#ifndef DHT20_H
#define DHT20_H

#include "main.h"
#include <stdint.h>

/*
  Số lần gọi dht20_update() giữa hai lần đo.
  dht20_update() được gọi mỗi 100 ms nên 20 lần = 2 giây giữa hai lần đo
  (đề yêu cầu đọc với chu kỳ hợp lý, không đọc liên tục)
*/
#define DHT20_PERIOD_CALLS  20U

// Khởi tạo cảm biến một lần lúc khởi động. Trả về 1 nếu thành công, 0 nếu lỗi
uint8_t  dht20_init(I2C_HandleTypeDef *hi2c);

// Task của scheduler, gọi mỗi 100 ms: tự gửi lệnh đo rồi đọc kết quả
void  dht20_update(void);
// Nhiệt độ lần đo gần nhất, đơn vị 0.1 độ C (có thể âm)
int16_t  dht20_get_temperature_x10(void);

// Độ ẩm lần đo gần nhất, đơn vị 0.1 %RH (0 đến 1000)
uint16_t dht20_get_humidity_x10(void);

// Trả về 1 nếu đã có ít nhất một lần đo hợp lệ (để LCD biết khi nào có số liệu thật)
uint8_t  dht20_is_valid(void);

#endif /* DHT20_H */