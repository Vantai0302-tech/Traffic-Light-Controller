/*
  dht20.c - Driver cảm biến DHT20. Xem dht20.h để biết cách dùng.

  Mỗi lần đo gồm hai bước, giữa hai bước phải chờ ít nhất 80 ms:
    Bước 1 (trigger): gửi lệnh 0xAC 0x33 0x00 để cảm biến bắt đầu đo.
    Bước 2 (read)   : đọc về 7 byte kết quả, có dạng:
        byte 0: trạng thái (bit 7 = 1 nghĩa là đang đo, chưa xong)
        byte 1: độ ẩm, bit 19..12
        byte 2: độ ẩm, bit 11..4
        byte 3: 4 bit cao = độ ẩm bit 3..0, 4 bit thấp = nhiệt độ bit 19..16
        byte 4: nhiệt độ, bit 15..8
        byte 5: nhiệt độ, bit 7..0
        byte 6: CRC-8 của 6 byte đầu
    Công thức đổi từ giá trị thô 20 bit sang đơn vị thật:
        độ ẩm (%RH)    = raw_h / 2^20 * 100
        nhiệt độ (độ C) = raw_t / 2^20 * 200 - 50
 
  Vì không được dùng HAL_Delay trong task, hai bước trên được chia ra hai lần
  gọi dht20_update() cách nhau 100 ms (lớn hơn 80 ms cần chờ).
 */
#include "dht20.h"

 
// địa chỉ I2C 7-bit của DHT20 là 0x38. HAL cần địa chỉ 8-bit nên dịch trái 1 bit (= 0x70)
#define DHT20_ADDR             (0x38U << 1)
// thời gian chờ tối đa cho một lần truyền I2C (ms) để không bị treo nếu cảm biến không trả lời
#define DHT20_I2C_TIMEOUT_MS   10U
// bit 7 của byte trạng thái bằng 1 nghĩa là cảm biến đang đo-> bận->dữ liệu chưa hợp lệ
#define DHT20_STATUS_BUSY      0x80U
 

 // hai bit hiệu chuẩn trong byte trạng thái, theo mã mẫu của nhà sản xuất
 // khi cả hai bit này bằng 1 thì cảm biến đã sẵn sàng đo 0001 1000
#define DHT20_STATUS_CAL_MASK  0x18U
 
 
// con trỏ tới handle I2C (hi2c1), được lưu lại lúc dht20_init()
static I2C_HandleTypeDef *dht20_i2c;
 
// biến lưu kết quả đo gần nhất, đơn vị 0.1 (ví dụ 253 = 25.3 độ C)
static int16_t  dht20_temp_x10;
static uint16_t dht20_hum_x10;
 
// cờ valid, bằng 1 sau khi có ít nhất một lần đọc hợp lệ
static uint8_t  dht20_valid;
 
// trạng thái của máy trạng thái trong dht20_update()
typedef enum {
    DHT20_STATE_IDLE = 0,   // đang chờ đến lượt đo tiếp theo       
    DHT20_STATE_MEASURING   // đã gửi lệnh đo, chờ đọc kết quả     
} Dht20State_t;
 
static Dht20State_t dht20_state;
 
// Đếm số lần dht20_update() đã gọi kể từ lần đo trước, dùng để tạo chu kỳ 2 giây
static uint8_t dht20_wait_calls;
  
//cảm biến gửi kèm 1 byte CRC (byte thứ 7 sau 6 byte data)->check xem kết quả có toàn vẹn ko
//nếu nhiễu làm sai 1 bit, CRC sẽ lệch, ta bỏ lần đọc đó, nhờ vậy LCD không bao giờ hiện số rác
static uint8_t dht20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;                                  // giá trị khởi tạo theo datasheet 
 
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];                                  // trộn byte dữ liệu vào CRC
        for (uint8_t b = 0; b < 8; b++) {                // xử lý lần lượt từng bit 
            if (crc & 0x80U) {
                crc = (uint8_t)(((uint32_t)crc << 1) ^ 0x31U);  // bit cao là 1: dịch và XOR đa thức 0x31 (theo datasheet)
            } else {
                crc = (uint8_t)((uint32_t)crc << 1);            // bit cao là 0: chỉ dịch trái
            }
        }
    }
    return crc;
}
 

//hàm dht20_calibrate_register hiệu chuẩn lại một thanh ghi của cảm biến.
//reg: địa chỉ thanh ghi cần hiệu chuẩn (0x1B, 0x1C hoặc 0x1E)
//trả về: 1 nếu thành công, 0 nếu lỗi I2C.
//Hàm này CHỈ được gọi khi cảm biến báo chưa hiệu chuẩn (hiếm gặp) và có dùng
//HAL_Delay nên chỉ chạy trong dht20_init() lúc khởi động, không chạy trong task

static uint8_t dht20_calibrate_register(uint8_t reg)
{
    // theo trình tự mẫu datasheet, trình tự là ghi {reg, 0x00, 0x00}, chờ 5 ms, đọc lại 3 byte, 
    // chờ 10 ms rồi ghi {0xB0 | reg, byte1, byte2}

    uint8_t tx[3] = {reg, 0x00, 0x00};    // bước 1: ghi thanh ghi = 0x00 0x00
    uint8_t rx[3] = {0};                  // chứa giá trị đọc lại: [trạng thái, byte1, byte2]
    
    //ghi, lỗi thì return 0
    if (HAL_I2C_Master_Transmit(dht20_i2c, DHT20_ADDR, tx, 3, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
    HAL_Delay(5);                           // chờ cảm biến xử lý (theo mã mẫu: 5 ms)
    
    //đọc lại 3 byte, lỗi thì return 0
    if (HAL_I2C_Master_Receive(dht20_i2c, DHT20_ADDR, rx, 3, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
    HAL_Delay(10);                          // chờ thêm (theo mã mẫu: 10 ms)
    
    tx[0] = (uint8_t)(0xB0U | reg);         // bước 2: ghi lại vào thanh ghi với đúng giá trị vừa đọc được
    tx[1] = rx[1];                         
    tx[2] = rx[2];
    //ghi, lỗi thì return 0
    if (HAL_I2C_Master_Transmit(dht20_i2c, DHT20_ADDR, tx, 3, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
    return 1;
}
 
//Bước 1 của 1 lần đo
//gửi 3 byte 0xAC 0x33 0x00. 0xAC là lệnh bắt đầu đo, hai byte 
//sau là tham số cố định theo datasheet. Hàm chỉ gửi lệnh và trả 
//về ngay, không chờ, vì chờ sẽ chặn chương trình
static uint8_t dht20_trigger(void)
{
    uint8_t cmd[3] = { 0xAC, 0x33, 0x00 };  // lệnh đo theo datasheet: 0xAC, 0x33, 0x00
 
    if (HAL_I2C_Master_Transmit(dht20_i2c, DHT20_ADDR, cmd, 3, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
    return 1;
}
 
//Bước 2 của một lần đo
//đọc 7 byte, lỗi I2C thì trả 0 và giữ giá trị cũ.
//kiểm tra bit bận, nếu cảm biến chưa đo xong thì bỏ lần này.
//kiểm tra CRC của 6 byte đầu so với byte thứ 7.
//ghép 20 bit, độ ẩm nằm ở byte 1, byte 2 và 4 bit cao của byte 3: (d1<<12)|(d2<<4)|(d3>>4). 
//nhiệt độ nằm ở 4 bit thấp của byte 3, byte 4 và byte 5: ((d3&0x0F)<<16)|(d4<<8)|d5. 
//hai giá trị dùng chung byte 3 nên phải tách bằng mask.
static uint8_t dht20_read(void)
{
    uint8_t  d[7];                          // 7 byte nhận về gồm trạng thái + dữ liệu + CRC
    uint32_t raw_h;                         // độ ẩm raw 20 bit
    uint32_t raw_t;                         // nhiệt độ raw 20 bit
    //đọc 7 byte, lỗi thì return 0
    if (HAL_I2C_Master_Receive(dht20_i2c, DHT20_ADDR, d, 7, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
 
    if (d[0] & DHT20_STATUS_BUSY) {
        return 0;                           // cảm biến chưa đo xong, bỏ qua lần này
    }
 
    if (dht20_crc8(d, 6) != d[6]) {
        return 0;                           // sai CRC: dữ liệu bị nhiễu, không dùng
    }
 
    // ghép độ ẩm 20 bit từ byte 1, byte 2 và 4 bit cao của byte 3, đọc từ MSB xuống
    raw_h = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | ((uint32_t)d[3] >> 4);
 
    // ghép nhiệt độ 20 bit từ 4 bit thấp của byte 3, byte 4 và byte 5
    raw_t = (((uint32_t)d[3] & 0x0FU) << 16) | ((uint32_t)d[4] << 8) | (uint32_t)d[5];
 
    /*
      Đổi sang đơn vị 0.1 bằng số nguyên (có làm tròn bằng cách cộng 2^19 trước khi dịch):
        độ ẩm x10    = raw_h * 1000 / 2^20
        nhiệt độ x10 = raw_t * 2000 / 2^20 - 500 
      Với giá trị thô 20 bit, các phép nhân trên không bị tràn số 32 bit.
     */
    //->đỡ dùng số thực để lưu kết quả
    dht20_hum_x10  = (uint16_t)((raw_h * 1000UL + (1UL << 19)) >> 20);
    dht20_temp_x10 = (int16_t)((int32_t)((raw_t * 2000UL + (1UL << 19)) >> 20) - 500);
 
    dht20_valid = 1; //xác nhận một lần đọc hợp lệ
    return 1;
}
 

 //hàm global
/*
  dht20_init - Khởi tạo cảm biến, gọi MỘT lần lúc khởi động (trước khi chạy scheduler).
    hi2c: handle I2C đang nối với DHT20 (&hi2c1)
    Trả về: 1 nếu cảm biến phản hồi và đã hiệu chuẩn, 0 nếu lỗi.
  Hàm có dùng HAL_Delay khoảng 100 ms nên không được gọi trong task.
 */
uint8_t dht20_init(I2C_HandleTypeDef *hi2c)
{
    uint8_t status = 0;
 
    // lưu handle I2C và đặt lại trạng thái ban đầu
    dht20_i2c        = hi2c;
    dht20_valid      = 0;
    dht20_state      = DHT20_STATE_IDLE;
    dht20_wait_calls = DHT20_PERIOD_CALLS;  // đặt bằng chu kỳ để lần gọi update() đầu tiên đo ngay
 
    HAL_Delay(100);                   // datasheet: chờ cảm biến ổn định >= 100 ms sau khi cấp điện
 
    // đọc 1 byte trạng thái, nếu không đọc được nghĩa là cảm biến không phản hồi (sai dây, sai khe I2C)
    if (HAL_I2C_Master_Receive(dht20_i2c, DHT20_ADDR, &status, 1, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
        return 0;
    }
 
    // nếu chưa hiệu chuẩn thì hiệu chuẩn lại ba thanh ghi 0x1B, 0x1C, 0x1E theo mã mẫu của nhà sản xuất
    if ((status & DHT20_STATUS_CAL_MASK) != DHT20_STATUS_CAL_MASK) {
        if (!dht20_calibrate_register(0x1B) ||!dht20_calibrate_register(0x1C)||!dht20_calibrate_register(0x1E)) {
            return 0;
        }
        HAL_Delay(10);                      /* chờ cảm biến cập nhật trước khi đọc lại trạng thái */
 
        //đọc lại trạng thái để kiểm tra đã hiệu chuẩn xong chưa
        if (HAL_I2C_Master_Receive(dht20_i2c, DHT20_ADDR, &status, 1, DHT20_I2C_TIMEOUT_MS) != HAL_OK) {
            return 0;
        }
        if ((status & DHT20_STATUS_CAL_MASK) != DHT20_STATUS_CAL_MASK) {
            return 0;                       //vẫn chưa hiệu chuẩn được-> cảm biến có vấn đề
        }
    }
 
    return 1;
}
 
/*
 dht20_update - Task của scheduler, gọi mỗi 100 ms (ví dụ SCH_Add_Task(dht20_update, 0, 10)).
 Hoạt động như một máy trạng thái hai trạng thái, không chặn chương trình:
   IDLE     : đếm đủ DHT20_PERIOD_CALLS lần gọi (2 giây) thì gửi lệnh đo, chuyển sang MEASURING.
   MEASURING: ở lần gọi kế tiếp (cách 100 ms, lớn hơn 80 ms cần chờ) thì đọc kết quả,
              rồi quay về IDLE dù đọc thành công hay không. Nếu lỗi thì giữ giá trị cũ
              và lần đo sau (2 giây nữa) sẽ thử lại.
 */
void dht20_update(void)
{
    switch (dht20_state) {
 
    case DHT20_STATE_IDLE:
        dht20_wait_calls++;                             // thêm một lần gọi vào bộ đếm chu kỳ
        if (dht20_wait_calls >= DHT20_PERIOD_CALLS) {   // đã đủ 2 giây (chạy 100ms đủ 20 lần)
            dht20_wait_calls = 0;
            if (dht20_trigger()) {                      // gửi lệnh đo thành công
                dht20_state = DHT20_STATE_MEASURING;    // lần gọi sau sẽ đọc kết quả
            }
        }
        break;
 
    case DHT20_STATE_MEASURING:
        (void)dht20_read();                             // đọc và cập nhật giá trị nếu hợp lệ
        dht20_state = DHT20_STATE_IDLE;                 // quay về chờ chu kỳ tiếp theo
        break;
    }
}
 
// Trả về nhiệt độ lần đo hợp lệ gần nhất, đơn vị 0.1 độ C (ví dụ 253 = 25.3 độ C)
int16_t dht20_get_temperature_x10(void)
{
    return dht20_temp_x10;
}
 
// Trả về độ ẩm lần đo hợp lệ gần nhất, đơn vị 0.1 %RH (ví dụ 604 = 60.4 %RH)
uint16_t dht20_get_humidity_x10(void)
{
    return dht20_hum_x10;
}
 
// Trả về 1 nếu đã có ít nhất một lần đo hợp lệ, 0 nếu chưa (giá trị khi đó là 0, chưa có nghĩa)
uint8_t dht20_is_valid(void)
{
    return dht20_valid;
}
 