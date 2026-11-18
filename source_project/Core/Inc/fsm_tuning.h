/*
  fsm_tuning.h - Máy trạng thái chế độ chỉnh thời gian đèn (hiển thị trên LCD)
 
  CÁCH DÙNG:
    fsm_mode gọi các hàm này, không gọi trực tiếp từ main():
      fsm_tuning_init() khi vào chế độ TUNING
      fsm_tuning_run()  mỗi 10 ms khi đang ở chế độ TUNING
      fsm_tuning_stop() khi rời chế độ TUNING
 
  PHÂN CÔNG NÚT (id nút bắt đầu từ 0, BTN_1 là id 0):
    id 1 (BTN_2): tăng giá trị 1 giây (hết 99 thì quay về 1); giữ nút 1 giây thì tăng thêm 10
    id 2 (BTN_3): chọn đèn cần chỉnh theo vòng Đỏ -> Xanh -> Vàng -> Đỏ
    id 3 (BTN_4): xác nhận, hệ thống kiểm tra ràng buộc rồi mới lưu
    id 0 (BTN_1): đổi chế độ, do fsm_mode xử lý
 
  AN TOÀN: ở chế độ này mọi đèn giao thông giữ màu Đỏ, nên không thể có 2 tuyến cùng Xanh.
  Giá trị chỉ được ghi vào biến chung khi bấm xác nhận và thỏa ràng buộc trong global.h.
*/
#ifndef FSM_TUNING_H
#define FSM_TUNING_H

// vào chế độ chỉnh: đèn về Đỏ, chọn đèn Đỏ, vẽ màn hình
void fsm_tuning_init(void);

// xử lý nút nhấn, cập nhật LCD, gọi mỗi 10 ms
void fsm_tuning_run(void);

// rời chế độ chỉnh: dừng timer, đèn về Đỏ
void fsm_tuning_stop(void);

#endif /* FSM_TUNING_H */
