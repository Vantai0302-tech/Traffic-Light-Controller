/*
  fsm_mode.c - Chuyển chế độ AUTO -> MANUAL -> TUNING. Xem fsm_mode.h để biết cách dùng.
 
  Máy trạng thái này KHÔNG gọi hàm HAL. Mọi thao tác đèn, nút, LCD đều đi qua module Bước 5.
*/
#include "fsm_mode.h"
#include "global.h"
#include "button.h"
#include "fsm_automatic.h"
#include "fsm_manual.h"
#include "fsm_tuning.h"
#include "display.h"

// nút đổi chế độ (BTN_1)
#define BTN_MODE_ID   0U

/*
  fsm_mode_clear_buttons - Xóa hết sự kiện nút còn tồn.
  Cờ nhấn được giữ cho tới khi có hàm đọc nó. Nếu không xóa, một lần bấm nút ở chế độ cũ
  (ví dụ nút 2 trong AUTO) sẽ bị tính vào chế độ mới ngay khi vừa chuyển.
 */
//hàm xoá hết sự kiện button còn tồn, reset cờ, bỏ qua return value
static void fsm_mode_clear_buttons(void)
{
    for (uint8_t i = 0U; i < BUTTON_COUNT; i++) {
        (void)isButtonPressed(i);
        (void)isButtonLongPressed(i);
    }
}

/*
  hàm dừng fsm của chế độ mode.
  Mỗi hàm stop đều đưa đèn về đỏ và dừng timer của nó.
*/
static void fsm_mode_stop(SystemMode mode)
{
    switch (mode) {
    case MODE_AUTO:   
        fsm_automatic_stop(); 
        break;
    case MODE_MANUAL: 
        fsm_manual_stop();    
        break;
    case MODE_TUNING: 
        fsm_tuning_stop();    
        break;
    default: 
        break;
    }
}

/*
  hàm vào chế độ mode: ghi vào biến chung, khởi động máy trạng thái và cập nhật LCD.
  display_refresh() tự bỏ qua ở TUNING vì fsm_tuning_init() đã vẽ màn hình riêng.
 */
static void fsm_mode_start(SystemMode mode)
{
    system_mode = mode;

    switch (mode) {
    case MODE_AUTO:   
        fsm_automatic_init(); 
        break;
    case MODE_MANUAL: 
        fsm_manual_init();    
        break;
    case MODE_TUNING: 
        fsm_tuning_init();    
        break;
    default: 
        break;
    }
    display_refresh();
}

/*
  hàm khởi động hệ thống ở chế độ AUTO, gọi một lần trước khi chạy scheduler.
 */
void fsm_mode_init(void)
{
    fsm_mode_clear_buttons(); //clear flag button
    fsm_mode_start(MODE_AUTO); //mặc định mới vô là AUTO
}

/*
  task của scheduler, gọi mỗi 10 ms.
  Nếu có bấm nút đổi chế độ thì dừng chế độ cũ rồi vào chế độ kế tiếp (AUTO -> MANUAL -> TUNING -> AUTO).
  Nếu không thì chạy máy trạng thái của chế độ hiện tại.
 */
void fsm_mode_run(void)
{
    SystemMode next;
    //coi có tín hiệu button đổi mode không
    if (isButtonPressed(BTN_MODE_ID)) {
        switch (system_mode) {
        case MODE_AUTO:   
            next = MODE_MANUAL; 
            break;
        case MODE_MANUAL: 
            next = MODE_TUNING; 
            break;
        case MODE_TUNING: 
            next = MODE_AUTO;   
            break;
        default:          
            next = MODE_AUTO;   
            break;
        }

        fsm_mode_stop(system_mode);         // đèn về Đỏ, dừng timer của chế độ cũ
        fsm_mode_clear_buttons();           // bỏ sự kiện nút còn tồn (3 button còn lại)
        fsm_mode_start(next);               // vào chế độ mới
        return;
    }

    switch (system_mode) {
    case MODE_AUTO:   
        fsm_automatic_run(); 
        break;
    case MODE_MANUAL: 
        fsm_manual_run();    
        break;
    case MODE_TUNING: 
        fsm_tuning_run();    
        break;
    default: 
        break;
    }
}
