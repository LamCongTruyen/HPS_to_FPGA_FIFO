# Đồ án tốt nghiệp - Học viện Công nghệ Bưu chính viễn thông
# Thiết kế và triển khai thuật toán AES trên FPGA

video quá trình cho điểm Hội đồng 3 lớp Vi mạch: https://www.youtube.com/watch?v=PYMLmPdxt6Y

<img width="428" height="647" alt="image" src="https://github.com/user-attachments/assets/27e5fe10-03a7-43ab-8488-2f09d7d235db" />

<img width="570" height="461" alt="image" src="https://github.com/user-attachments/assets/7599be84-4121-4fbb-a817-3a9fda7aadc3" />

Cấu hình Platform Design (Qsys):

<img width="1916" height="1050" alt="image" src="https://github.com/user-attachments/assets/40997467-6cdd-478d-8a0d-df1efd86243d" />


Tôi tìm hiểu và xây dựng dự án đầu tiên trên dòng DE1-SoC, với ý tưởng làm quen trên dòng SoC FPGA này là boot Linux trên HPS và truyền nhận dữ liệu xuống FPGA qua các IP trong Qsys (Platform Design trong Quatus Prime).

Dòng SoCFPGA này cũng cho phép chạy một webserver bằng ngôn ngữ HTML nhưng vì tôi không thành thạo ngôn ngữ này nên đã chạy một server hết sức đơn giản bằng Python trên laptop Window sau đó gửi hình ảnh từ server tới HPS qua cổng Ethernet được kết nối trong mạng cục bộ. HPS nhận hình ảnh và xử lý bằng chương trình C chuyển hình ảnh thành các byte sau đó ánh xạ bộ nhớ tới các địa chỉ được khai báo ở đầu chương trình. Các địa chỉ này sinh ra trong quá trình Generate HDL trong Qsys nằm trong vùng bộ nhớ mặc định của FPGA được nhắc tới trong mục2:

https://ftp.intel.com/Public/Pub/fpgaup/pub/Intel_Material/18.1/Computer_Systems/DE1-SoC/DE1-SoC_Computer_NiosII.pdf

Khi làm việc với dòng SoC này tôi tham khảo phần lớn tài liệu từ dự án Cornell ece5760 theo tôi tìm hiểu thì đây là một khóa đào tạo của đại học Cornell với sự hỗ trợ của Intel. Dự án hướng dẫn cho người học về lí thuyết các IP trong Qsys đi kèm chương trình mẫu, đường link dự án : 

https://people.ece.cornell.edu/land/courses/ece5760/DE1_SOC/HPS_peripherials/FPGA_addr_index.html

Trang Youtube của BruceLand (một bên đồng hành cùng dự án) :

https://www.youtube.com/@ece4760

Trong phần giao tiếp HPS - FPGA mà trong đường link trên đề cập có 4 cách chính, tôi chọn cách mà tôi nghĩ là phù hợp với mình đó là "HPS to FPGA FIFO with feedback via SRAM scratchpad". Có thể hiểu là HPS giao tiếp với FPGA thông qua IP FIFO trong Qsys, FPGA làm gì đó với dữ liệu và ghi vào vùng SRAM chia sẽ chung giữa HPS và FPGA. HPS 'nhìn thấy' vùng nhớ đó là lấy dữ liệu thực hiện hoặc lưu trữ. Trước đó tôi cũng thử dùng "Full FIFO communication: HPS-to-FPGA and FPGA-to-HPS" nhưng việc bắt tay giữa các module thất bại nên tôi chọn cách mà tôi cho rằng mình nắm rõ hơn là "HPS to FPGA FIFO with feedback via SRAM scratchpad".

Tham khảo cách giao tiếp bên trong dự án tôi áp dụng vào dự án của mình như sau : trên HPS với Linux kernel 3.9 thì tôi xây dựng một chương trình C cho phép nhận ảnh từ webserver local (built đơn giản với python) sau đó chuyển hình ảnh ở dạng file nén jpg hoặc png dữ liệu byte. Ghi lần lượt dữ liệu ảnh xuống FPGA qua công cụ mmap ,FPGA nhận dữ liệu và đưa vào module AES_CTR để giải mã. Với mã hóa thì cũng thực hiện tương tự như vậy.

Kết quả chạy trên phần cứng thực tế cho tốc độ mã hóa cũng khá ấn tượng:

<img width="592" height="378" alt="image" src="https://github.com/user-attachments/assets/30dac101-77a0-4c41-9874-f4dc7bef481a" />

Video chạy thực tế trên phần cứng mà tôi đã thực hiện :

https://youtu.be/Q3kLE4kaFmQ

Sau khi xác minh thiết kế các module nhỏ bằng Testbench trên Modelsim (tôi có để hình ảnh trong dự án AES_uart_interface) và triển khi trên phần cứng chạy cho kết quả chính xác (tổng quát) để soi ra được xem có lỗi nào tiềm ẩn trong quá trình chạy (tại vì nếu lỗi 1,2byte thì không thể nào mà mở file dữ liệu ra đối chiếu từng byte được), thì tôi cũng thực hiện thêm đó là giám sát tín hiệu dữ liệu realtime thực tế được ghi lại vào Ram trong quá trình chạy trên kit bằng công cụ Signal Tap Logic Analyzer có trong Quatus. Kết quả sơ bộ như hình dưới:

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/e36e9aad-f351-40e3-9c92-3d3d0cf435fa" />

Thời gian hoàn thành mã hóa 1block 128bit: ~1.2us

<img width="1922" height="259" alt="image" src="https://github.com/user-attachments/assets/2829e11f-8cde-479a-bd01-eb230b901905" />

Thời gian hoàn thành giải mã 1block 128bit : ~1.2us

<img width="1913" height="243" alt="image" src="https://github.com/user-attachments/assets/9d7e1cf3-1b9d-4dda-8d0c-9a64d08e6370" />

Theo như dạng sóng mà các tín hiệu chạy thực tế theo thời gian thực trên kit được ghi lại thì tôi không phát hiện trường hợp nào bị lỗi vì tín hiệu đến trễ hay đến sớm hay là bị glitch. 

À, một số ràng buộc về timing của SocFPGA vì thiếu tài liệu chính xác phải nên cài đặt như thế nào nên tôi sử dụng mặc định theo cài đặt của dự án gốc trong tutorial của Terasic.

Dự án này nối tiếp dự án UART on FPGA trước đó của tôi nên bạn sẽ thấy có 1 vài file không liên quan lắm ^^.
