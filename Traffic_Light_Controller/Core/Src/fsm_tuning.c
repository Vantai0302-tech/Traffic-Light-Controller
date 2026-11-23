/*
  fsm_tuning.c - Máy trạng thái chỉnh thời gian đèn. Xem fsm_tuning.h để biết cách dùng.
 
  Màn hình LCD:
       Chinh: XANH        <- đèn đang chọn (DO, XANH, VANG)
       Gia tri: 8 s      <- giá trị đang chỉnh, hoặc thông báo "Da luu!" / "Gia tri sai!"
 
  Đèn: tuyến 1 nháy 2 Hz (sáng 250 ms, tắt 250 ms) theo màu đang chọn (Đỏ, Xanh, Vàng),
  tuyến 2 luôn giữ Đỏ nên không bao giờ có 2 tuyến cùng Xanh.

  Máy trạng thái này KHÔNG gọi hàm HAL, chỉ gọi hàm của button, traffic_light, display, global.
 */
#include "fsm_tuning.h"
#include "global.h"
#include "button.h"
#include "software_timer.h"
#include "traffic_light.h"
#include "display.h"
#include <stddef.h>

// timer dùng để tự tắt thông báo trên LCD (timer 0 đã dùng cho fsm_automatic)
#define TUNE_TIMER_ID       1U
#define TUNE_MESSAGE_MS     1500U //thông báo hiện 1.5s (optional)

// timer dùng để nháy đèn đang chỉnh
#define TUNE_BLINK_TIMER_ID 2U
#define TUNE_BLINK_MS       250U    //đổi trạng thái sáng/tắt mỗi 250 ms -> nháy 2 Hz

// phân công nút (id bắt đầu từ 0)
#define BTN_INCREASE_ID     1U      //nút tăng giá trị thời gian
#define BTN_DECREASE_ID     2U      //nút giảm giá trị thời gian
#define BTN_SELECT_ID       3U      //nút đổi led muốn tăng
#define BTN_CONFIRM_ID      4U      //nút xác nhận là lưu kết quả thay đổi (nút USER)


static uint8_t     tuning_active = 0U;      // cờ chặn fsm_tuning_run chạy khi chưa vào chế độ hoặc đã rời
static uint8_t     blink_on = 1U;           // 1 là pha sáng, 0 là pha tắt của đèn đang nháy
static TuneLight   tune_light;              // đèn đang chọn
static uint8_t     tune_value;              // giá trị đang chỉnh, chưa lưu cho tới khi xác nhận
static const char *tune_message = NULL;     // thông báo đang hiện ở dòng 1, NULL nếu không có (tức là hiển thị thời gian đang chỉnh)


//tên đèn để hiện trên LCD (không dấu).

static const char *tuning_light_name(TuneLight light)
{
    switch (light) {
    case TUNE_RED:    
        return "DO";
    case TUNE_GREEN:  
        return "XANH";
    case TUNE_YELLOW: 
        return "VANG";
    default:          
        return "";
    }
}


//hàm để nối chuỗi src vào cuối mảng dst, n là độ dài hiện có của dst.
//Dùng để ghép "Chinh: " với tên đèn mà không cần sprintf.
static void tuning_append(char *dst, uint8_t *n, const char *src)
{
    while (*src != '\0') {
        dst[*n] = *src;
        (*n)++;
        src++;
    }
    dst[*n] = '\0';
}

/*
  hàm vẽ lại cả hai dòng của màn hình chỉnh thời gian.
  Dòng 0 là đèn đang chọn, dòng 1 là thông báo (nếu có) hoặc giá trị đang chỉnh.
  Chỉ gọi khi có thay đổi (bấm nút, hết thời gian thông báo) để không chiếm CPU: mỗi ký tự mất khoảng 0.5 ms.
 */
static void tuning_draw(void)
{
    char line[DISPLAY_LINE_WIDTH + 1U];
    uint8_t n = 0U;

    // dòng 0: "Chinh: XANH"
    line[0] = '\0';
    tuning_append(line, &n, "Chinh: "); //in chỉnh
    tuning_append(line, &n, tuning_light_name(tune_light));//in ký tự màu đèn
    display_print_line(0U, line);

    // dòng 1: thông báo hoặc "Gia tri: 8 s"
    if (tune_message != NULL) {
        display_print_line(1U, tune_message);
        return;
    }
    n = 0U;
    line[0] = '\0';
    tuning_append(line, &n, "Gia tri: ");
    if (tune_value >= 10U) { //số có hai chữ số thì in bình thuòng, còn nếu có 1 chữ số thì chỉ in 1 số
        line[n] = (char)('0' + (tune_value / 10U));   // chữ số hàng chục
        n++;
    }
    line[n] = (char)('0' + (tune_value % 10U));       // chữ số hàng đơn vị
    n++;
    line[n]   = '\0';
    tuning_append(line, &n, " s");
    display_print_line(1U, line);
}


/*
  hàm xuất đèn theo pha nháy hiện tại.
  Tuyến 1 nháy theo màu đang chọn (Đỏ, Xanh hoặc Vàng), tuyến 2 luôn giữ Đỏ.
 */
static void tuning_blink_apply(void)
{
    TrafficColor color = OFF;

    if (blink_on != 0U) {
        switch (tune_light) {
        case TUNE_RED:
            color = RED;
            break;
        case TUNE_GREEN:
            color = GREEN;
            break;
        case TUNE_YELLOW:
            color = YELLOW;
            break;
        default:
            color = RED;
            break;
        }
    }

    // tuyến 2 luôn Đỏ, ghi trước rồi mới ghi tuyến 1 để không có lúc tuyến 1 đã Xanh mà tuyến 2 còn đang đổi
    traffic_set(2U, RED);
    traffic_set(1U, color);
}

// hàm bắt đầu nháy lại từ pha sáng, dùng khi vào chế độ hoặc khi đổi đèn để đèn mới sáng ngay
static void tuning_blink_restart(void)
{
    blink_on = 1U;
    tuning_blink_apply();
    setTimer(TUNE_BLINK_TIMER_ID, TUNE_BLINK_MS);
}


//hàm vào chế độ chỉnh thời gian.
//Chọn đèn Đỏ làm đèn đầu tiên, nạp giá trị hiện tại của nó và bắt đầu nháy đèn đó.

void fsm_tuning_init(void)
{
    tuning_active = 1U;
    tune_light    = TUNE_RED;
    tune_value    = global_get_time(TUNE_RED);
    tune_message  = NULL;

    tuning_blink_restart();
    tuning_draw();
}


 //hàm xử lý nút nhấn trong chế độ chỉnh, gọi mỗi 10 ms.
 //Mỗi sự kiện nút (isButtonPressed đọc xong là xóa cờ) chỉ được xử lý đúng một lần.

void fsm_tuning_run(void)
{
    if (tuning_active == 0U) {
        return;
    }

    // hết thời gian hiện thông báo -> quay lại hiện giá trị
    // (chỉ kiểm tra cờ khi đang có thông báo vì timer_flag của timer rảnh có thể bằng 1)
    if (tune_message != NULL && timer_flag[TUNE_TIMER_ID] == 1U) {
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_draw();
    }

    // đến lúc đổi pha nháy (timer rảnh có thể có cờ bằng 1 nên chỉ kiểm tra khi đang chỉnh, timer này luôn được đặt lại)
    if (timer_flag[TUNE_BLINK_TIMER_ID] == 1U) {
        blink_on = (blink_on == 0U) ? 1U : 0U;
        tuning_blink_apply();
        setTimer(TUNE_BLINK_TIMER_ID, TUNE_BLINK_MS);
    }

    // nút chọn đèn: Đỏ -> Xanh -> Vàng -> Đỏ, bỏ giá trị chưa xác nhận và nạp giá trị đã lưu
    if (isButtonPressed(BTN_SELECT_ID)) {
        switch (tune_light) {
        case TUNE_RED:    
            tune_light = TUNE_GREEN;  
            break;
        case TUNE_GREEN:  
            tune_light = TUNE_YELLOW; 
            break;
        case TUNE_YELLOW: 
            tune_light = TUNE_RED;    
            break;
        default:          
            tune_light = TUNE_RED;    
            break;
        }
        tune_value   = global_get_time(tune_light);
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_blink_restart();
        tuning_draw();
    }

    // nút tăng: +1 giây, hết 99 thì quay về 1
    if (isButtonPressed(BTN_INCREASE_ID)) {
        if (tune_value >= TIME_MAX_S) {
            tune_value = TIME_MIN_S;
        } else {
            tune_value++;
        }
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_draw();
    }

    // giữ nút tăng đủ 1 giây: tăng thêm 10 giây để không phải bấm quá nhiều lần
    if (isButtonLongPressed(BTN_INCREASE_ID)) {
        if (tune_value + 10U > TIME_MAX_S) {
            tune_value = TIME_MIN_S;
        } else {
            tune_value = (uint8_t)(tune_value + 10U);
        }
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_draw();
    }

    // nút giảm: -1 giây, xuống dưới 1 thì quay về 99
    if (isButtonPressed(BTN_DECREASE_ID)) {
        if (tune_value <= TIME_MIN_S) {
            tune_value = TIME_MAX_S;
        } else {
            tune_value--;
        }
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_draw();
    }

    // giữ nút giảm đủ 1 giây: giảm thêm 10 giây, xuống dưới 1 thì quay về 99
    if (isButtonLongPressed(BTN_DECREASE_ID)) {
        if (tune_value <= 10U) {
            tune_value = TIME_MAX_S;
        } else {
            tune_value = (uint8_t)(tune_value - 10U);
        }
        tune_message = NULL;
        timerStop(TUNE_TIMER_ID);
        tuning_draw();
    }

    // nút xác nhận: kiểm tra ràng buộc trong global_try_set rồi báo kết quả
    if (isButtonPressed(BTN_CONFIRM_ID)) {
        if (global_try_set(tune_light, tune_value) != 0U) {
            tune_message = "Da luu!";       // hợp lệ, đã lưu
        } else {
            tune_message = "Gia tri sai!";       // vi phạm ràng buộc, giữ nguyên giá trị cũ
        }
        setTimer(TUNE_TIMER_ID, TUNE_MESSAGE_MS);
        tuning_draw();
    }
}


 ///fsm_tuning_stop - Rời chế độ chỉnh thời gian.
 //Giá trị chưa xác nhận bị bỏ. Dừng nháy, đèn về Đỏ để chế độ tiếp theo bắt đầu từ trạng thái an toàn.

void fsm_tuning_stop(void)
{
    tuning_active = 0U;
    tune_message  = NULL;
    timerStop(TUNE_TIMER_ID);
    timerStop(TUNE_BLINK_TIMER_ID);
    traffic_all_red();
}