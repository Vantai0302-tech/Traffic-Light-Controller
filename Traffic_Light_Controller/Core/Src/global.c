/*
global.c - Biến dùng chung và kiểm tra ràng buộc thời gian đèn. Xem global.h để biết cách dùng.
 */
#include "global.h"

// chế độ hiện tại và thời gian từng đèn (giây)
SystemMode system_mode = MODE_AUTO;
uint8_t    time_red    = TIME_DEFAULT_RED_S;
uint8_t    time_green  = TIME_DEFAULT_GREEN_S;
uint8_t    time_yellow = TIME_DEFAULT_YELLOW_S;


//global_init đặt lại giá trị ban đầu, gọi một lần lúc khởi động.
//Mặc định Đỏ = Xanh + Vàng (10 = 8 + 2) để thỏa ràng buộc ngay từ đầu.
 
void global_init(void)
{
    system_mode = MODE_AUTO;
    time_red    = TIME_DEFAULT_RED_S;
    time_green  = TIME_DEFAULT_GREEN_S;
    time_yellow = TIME_DEFAULT_YELLOW_S;
}


//hàm lấy thời gian hiện tại của một đèn.
// light: TUNE_RED, TUNE_GREEN hoặc TUNE_YELLOW
// trả về thời gian (giây)
uint8_t global_get_time(TuneLight light)
{
    switch (light) {
    case TUNE_RED:    
        return time_red;
    case TUNE_GREEN:  
        return time_green;
    case TUNE_YELLOW: 
        return time_yellow;
    default:           
        return 0U;
    }
}


  //hàm nhận vào đèn cần đổi và thời gian sau khi điều chỉnh, copy giá trị 3 đèn ra check 
  //thử rule ban đầu coi thoả thì mới ghi đè giá trị mới này vào
uint8_t global_try_set(TuneLight light, uint8_t value)
{
    uint8_t r = time_red;
    uint8_t g = time_green;
    uint8_t y = time_yellow;

    if (value < TIME_MIN_S || value > TIME_MAX_S) {
        return 0U;                          // ngoài khoảng 1 -> 99
    }

    switch (light) {
    case TUNE_RED:          //đổi đỏ thì giữ vàng, xanh = đỏ - vàng, hai case còn lại thì tính lại đỏ
        if (value <= y) {
            return 0U;                      
        }
        r = value;
        g = (uint8_t)(r - y);
        break;

    case TUNE_GREEN:            
        g = value;
        r = (uint8_t)(g + y);               // tối đa 99 + 99 = 198, vẫn vừa uint8_t
        break;

    case TUNE_YELLOW:
        y = value;
        r = (uint8_t)(g + y);
        break;

    default:
        return 0U;
    }

    // kiểm tra ràng buộc trên bộ ba mới
    if (r > TIME_MAX_S || g < TIME_MIN_S || y < TIME_MIN_S) {
        return 0U;                          // Đỏ vượt 99 giây
    }
    if (y >= g) {
        return 0U;                          // Vàng phải ngắn hơn Xanh
    }

    time_red    = r;                        // hợp lệ thì mới ghi vào biến chung
    time_green  = g;
    time_yellow = y;
    return 1U;
}
