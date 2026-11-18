/*
  fsm_mode.h - Máy trạng thái chuyển giữa 3 chế độ lớn: AUTO -> MANUAL -> TUNING -> AUTO
 
  CÁCH DÙNG:
    1. Trong main(), sau khi đã khởi tạo timer, button, traffic_light, global, lcd:
           fsm_mode_init();
    2. Đăng ký task chu kỳ 10 ms vào scheduler (tick 10 ms nên là 1 tick):
           SCH_Add_Task(fsm_mode_run, 0, 1);
       Task này tự gọi hàm chạy của máy trạng thái đang hoạt động (fsm_automatic, fsm_manual, fsm_tuning).
    3. Nút id 0 (BTN_1) bấm một lần để sang chế độ kế tiếp.
 
  KHI ĐỔI CHẾ ĐỘ (trả lời câu hỏi trọng tâm của đề):
    - Đèn 2 tuyến: dừng chế độ cũ -> cả hai tuyến về Đỏ -> chế độ mới tự đặt trạng thái đầu tiên của nó.
      Mỗi trạng thái đều xuất Đỏ cho cả hai tuyến trước rồi mới bật đèn riêng, nên không bao giờ có 2 tuyến cùng Xanh.
    - Timer: chế độ cũ dừng timer của nó (timerStop), chế độ mới đặt lại timer từ đầu.
    - Nút: xóa hết sự kiện nút còn tồn để bấm của chế độ cũ không bị tính sang chế độ mới.
    - LCD: cập nhật ngay tên chế độ (hoặc màn hình chỉnh thời gian).
  Lưu ý: nếu đổi chế độ lúc đang Vàng thì đèn chuyển thẳng về Đỏ, không giữ nốt đèn Vàng.
 */
#ifndef FSM_MODE_H
#define FSM_MODE_H

// khởi động hệ thống ở chế độ AUTO
void fsm_mode_init(void);

// xử lý nút đổi chế độ, rồi chạy máy trạng thái của chế độ hiện tại, gọi mỗi 10 ms
void fsm_mode_run(void);

#endif /* FSM_MODE_H */
