# First_prj_on_DE1-Soc
<img width="428" height="647" alt="image" src="https://github.com/user-attachments/assets/27e5fe10-03a7-43ab-8488-2f09d7d235db" />

<img width="570" height="461" alt="image" src="https://github.com/user-attachments/assets/7599be84-4121-4fbb-a817-3a9fda7aadc3" />

Tôi tìm hiểu và xây dựng dự án đầu tiên trên dòng DE1-SoC, với ý tưởng làm quen trên dòng SoC FPGA này là boot Linux trên HPS và truyền nhận dữ liệu xuống FPGA qua các IP tỏng Qsys (Platform Design trong Quatus Prime).

Dòng SoCFPGA này cũng cho phép chạy một webserver bằng ngôn ngữ HTML nhưng vì tôi không thành thạo ngôn ngữ này nên đã chạy một server hết sức đơn giản bằng Python trên laptop Window sau đó gửi hình ảnh từ server tới HPS qua cổng Ethernet được kết nối trong mạng cục bộ. HPS nhận hình ảnh và xử lý bằng chương trình C chuyển hình ảnh thành các byte sau đó ánh xạ bộ nhớ tới các địa chỉ được khai báo ở đầu chương trình. Các địa chỉ này sinh ra trong quá trình Generate HDL trong Qsys nằm trong vùng bộ nhớ mặc định của FPGA được nhắc tới trong mục 2:https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/18.1/Computer_Systems/DE1-SoC/DE1-SoC_Computer_NiosII.pdf

Một số ràng buộc về "Timing" của SocFPGA vì thiếu tài liệu chính xác phải nên cài đặt như thế nào nên tôi sử dụng mặc định theo cài đặt của dự án gốc trong tutorial của Terasic.
