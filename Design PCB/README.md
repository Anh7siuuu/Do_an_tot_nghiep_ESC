# Thiết Kế và Chế Tạo Mạch Điều Khiển Động Cơ (ESC) Cho BLDC Của UAV 🚁

Repository này chứa mã nguồn và các tài liệu liên quan đến dự án phát triển Mạch điều khiển tốc độ điện tử (Electronic Speed Controller - ESC) dành cho động cơ Brushless DC (BLDC) ứng dụng trên máy bay không người lái (UAV). Mục tiêu của dự án là thiết kế một hệ thống tối ưu hóa hiệu năng hoạt động của động cơ, đảm bảo khả năng phản hồi nhanh và ổn định.

Dự án này là một phần của Đồ án tốt nghiệp tại **Đại học Bách Khoa Hà Nội (HUST)**.
* **Sinh viên thực hiện:** Trương Quang Thái (20224139) & Nguyễn Duy Hải Nam (20224334)
* **Giảng viên hướng dẫn:** PGS.TS. GVCC Nguyễn Hữu Phát

---

## 📂 Cấu trúc Repository

Dự án được chia thành hai thành phần phần mềm chính để kiểm thử và điều khiển toàn bộ hệ thống:

* **`PCB_TEST/`**: Thư mục chứa mã nguồn thuật toán điều khiển cốt lõi được nạp trực tiếp lên vi điều khiển của ESC. Chịu trách nhiệm thực thi các thuật toán chuyển mạch (commutation), kiểm soát dòng/áp và điều khiển động cơ BLDC hoạt động với hiệu suất tối ưu.
* **`motherboard_controller/`**: Thư mục chứa mã nguồn cho bo mạch điều khiển trung tâm (Motherboard). Đảm nhiệm việc đọc giá trị từ biến trở (potentiometer) để lấy tín hiệu đầu vào từ người dùng, sau đó tiến hành điều chế xung (PWM) và gửi tín hiệu điều khiển này tới ESC.

---

## 🚀 Tính năng chính

* **Điều khiển động cơ BLDC:** Thuật toán tối ưu giúp động cơ hoạt động mượt mà, phản hồi tốt với các thay đổi tốc độ.
* **Đọc tín hiệu & Điều chế PWM chính xác:** Xử lý tín hiệu analog từ biến trở thông qua ADC và tạo ra chuỗi xung PWM tần số cao với độ trễ thấp để giao tiếp với ESC.
* **Khả năng mở rộng:** Thiết kế phần mềm theo dạng module, dễ dàng tùy chỉnh thông số và tích hợp thêm các giao thức giao tiếp mới.

---

## 🛠️ Yêu cầu phần cứng & Phần mềm

### Phần cứng
* Bo mạch ESC tự thiết kế (tương ứng với mã trong `PCB_TEST`).
* Bo mạch vi điều khiển (Motherboard) để chạy mã tạo PWM.
* Động cơ BLDC dành cho UAV.
* Nguồn DC (Pin LiPo 3S-6S tùy thuộc vào cấu hình).
* Biến trở (Potentiometer) để kiểm thử thay đổi tốc độ thủ công.

### Phần mềm
* IDE và Trình biên dịch tương ứng với dòng vi điều khiển được sử dụng (ví dụ: STM32CubeIDE, Keil C, hoặc Arduino IDE).
* Phần mềm nạp code (ST-Link, J-Link...).

---

## 🔧 Hướng dẫn sử dụng

1.  **Clone repository về máy:**
    ```bash
    git clone https://github.com/Anh7siuuu/Do_an_tot_nghiep_ESC
    ```
2.  **Cài đặt cho Motherboard:**
    * Mở project trong thư mục `motherboard_controller/`.
    * Biên dịch và nạp mã nguồn xuống bo mạch điều khiển.
    * Đảm bảo biến trở đã được kết nối đúng vào chân ADC đã cấu hình.
3.  **Cài đặt cho ESC:**
    * Mở project trong thư mục `PCB_TEST/`.
    * Biên dịch và nạp mã nguồn xuống mạch ESC.
    * Kiểm tra kết nối các pha của động cơ BLDC với mạch ESC.
4.  **Vận hành thử nghiệm:**
    * Cấp nguồn cho toàn bộ hệ thống (Lưu ý: Luôn tuân thủ các quy tắc an toàn khi làm việc với động cơ công suất lớn và pin LiPo).
    * Vặn biến trở từ từ để quan sát sự thay đổi độ rộng xung PWM và tốc độ phản hồi tương ứng của động cơ BLDC.

---

## 🔮 Hướng phát triển trong tương lai (Future Works)

Để đáp ứng nhu cầu giao tiếp ngày càng phức tạp giữa ESC và Motherboard của các dòng UAV hiện đại, dự án có định hướng phát triển thêm:
* Tối ưu hóa lại kích thước bản mạch PCB.
* Tích hợp các chuẩn giao tiếp kỹ thuật số như **UART** để thu thập telemetry (dữ liệu viễn trắc) khi bay.
* Hỗ trợ các giao thức điều khiển tốc độ cao như **DShot** hoặc **MultiShot** nhằm tăng khả năng tương thích với nhiều loại Flight Controller khác nhau trên thị trường.

---

## 📄 Tài liệu tham khảo
* [UM2197] Electronic speed controller for BLDC and PMSM three phase brushless motor.
* S.-H. Kim, *Electric Motor Control: DC, AC, and BLDC Motors*, 2017.
* Và các tài liệu chuyên ngành khác (xem chi tiết trong báo cáo đồ án).
