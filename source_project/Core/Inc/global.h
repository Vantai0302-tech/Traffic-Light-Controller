/*
 * global.h - Biến dùng chung cho toàn hệ thống (theo đề: trạng thái hiện tại, thời gian Đỏ/Xanh/Vàng)
 *
 * CÁCH DÙNG:
 *   1. Trong main(), TRƯỚC khi chạy các máy trạng thái:
 *          global_init();
 *   2. fsm_automatic đọc thời gian đèn từ time_green, time_yellow (đơn vị giây).
 *   3. fsm_tuning KHÔNG ghi trực tiếp vào các biến này mà gọi global_try_set(),
 *      hàm sẽ kiểm tra ràng buộc rồi mới cập nhật.
 *
 * RÀNG BUỘC THỜI GIAN (đề yêu cầu "quy định ràng buộc hợp lệ giữa Đỏ, Xanh, Vàng"):
 *   1. Mỗi giá trị nằm trong khoảng TIME_MIN_S .. TIME_MAX_S (1 -> 99 giây).
 *   2. Đỏ = Xanh + Vàng, vì khi một tuyến đỏ thì tuyến kia chạy hết xanh rồi vàng.
 *      Nhờ vậy hai tuyến luôn khớp nhau và không bao giờ có 2 tuyến cùng xanh.
 *   3. Vàng < Xanh (đèn vàng chỉ để báo sắp đổi nên phải ngắn hơn xanh).
 */
#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

// các chế độ lớn của hệ thống
typedef enum {
    MODE_AUTO = 0,      // chạy tự động theo thời gian
    MODE_MANUAL,        // chuyển trạng thái đèn bằng nút nhấn
    MODE_TUNING         // chỉnh thời gian đèn, hiển thị trên LCD
} SystemMode;

// đèn đang được chọn để chỉnh trong fsm_tuning
typedef enum {
    TUNE_RED = 0,
    TUNE_GREEN,
    TUNE_YELLOW
} TuneLight;

// giới hạn và giá trị mặc định (đơn vị giây)
#define TIME_MIN_S             1U
#define TIME_MAX_S             99U
#define TIME_DEFAULT_RED_S     10U
#define TIME_DEFAULT_GREEN_S   8U
#define TIME_DEFAULT_YELLOW_S  2U

// chế độ hiện tại và thời gian từng đèn (giây)
extern SystemMode system_mode;
extern uint8_t    time_red;
extern uint8_t    time_green;
extern uint8_t    time_yellow;

// đặt lại chế độ về MODE_AUTO và thời gian về giá trị mặc định
void global_init(void);

// thử đặt thời gian cho một đèn, trả về 1 nếu hợp lệ (đã cập nhật), 0 nếu vi 
// phạm ràng buộc (giữ nguyên giá trị cũ) đỏ=vàng+xanh
uint8_t global_try_set(TuneLight light, uint8_t value);

// trả về thời gian hiện tại (giây) của đèn được chọn
uint8_t global_get_time(TuneLight light);

#endif /* GLOBAL_H */
