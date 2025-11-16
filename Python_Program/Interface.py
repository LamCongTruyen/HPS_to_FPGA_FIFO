import sys
import socket
import cv2
import base64
import numpy as np
from PyQt5.QtWidgets import (
    QApplication, QWidget, QPushButton, QLabel, QVBoxLayout,
    QHBoxLayout, QFileDialog
)
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtCore import Qt, QTimer

SERVER_IP = "192.168.100.2"
PORT = 8080
BUFFER_SIZE = 4096
CAM_URL = "http://xxx.xxx.x.xxx:xx/stream"


class App(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("AES Encrypt/Decrypt Image - DE1-SOC")
        self.setGeometry(100, 100, 1200, 600)

        self.image_bytes = None
        self.image_path = None
        self.cap = None

        self.original_label = QLabel("Ảnh gốc")
        self.original_label.setAlignment(Qt.AlignCenter)
        self.original_label.setMinimumSize(300, 300)
        self.original_label.setStyleSheet("border: 1px solid gray; background: #f0f0f0")

        self.result_label = QLabel("Ảnh sau giải mã")
        self.result_label.setAlignment(Qt.AlignCenter)
        self.result_label.setMinimumSize(300, 300)
        self.result_label.setStyleSheet("border: 1px solid gray; background: #f0f0f0")

        self.cam_label = QLabel("Camera Stream")
        self.cam_label.setAlignment(Qt.AlignCenter)
        self.cam_label.setMinimumSize(400, 300)
        self.cam_label.setStyleSheet("border: 1px solid gray; background: #000")

        self.status_label = QLabel("Status: Đang khởi động camera...")
        self.status_label.setAlignment(Qt.AlignCenter)

        self.btn_load = QPushButton("Load Image")
        self.btn_send = QPushButton("Send to DE1-SOC")
        self.btn_capture = QPushButton("Capture từ Camera")

        self.btn_load.clicked.connect(self.load_image)
        self.btn_send.clicked.connect(self.send_image)
        self.btn_capture.clicked.connect(self.capture_from_stream)

        self.start_camera_stream()

        self.setup_ui()

    def setup_ui(self):
  
        img_row = QHBoxLayout()
        img_row.addWidget(self.original_label)
        img_row.addWidget(self.result_label)
        img_row.addWidget(self.cam_label)

        btn_row = QHBoxLayout()
        btn_row.addWidget(self.btn_load)
        btn_row.addWidget(self.btn_send)
        btn_row.addWidget(self.btn_capture)

        main_layout = QVBoxLayout()
        main_layout.addLayout(img_row)
        main_layout.addLayout(btn_row)
        main_layout.addWidget(self.status_label)
        self.setLayout(main_layout)

    def start_camera_stream(self):
        """Khởi động stream camera và timer"""
        self.cap = cv2.VideoCapture(CAM_URL)
        if not self.cap.isOpened():
            self.status_label.setText("Lỗi: Không kết nối được ESP32-CAM!")
            return

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_stream)
        self.timer.start() 
        self.status_label.setText("Camera đang stream...")

    def update_stream(self):
        """Cập nhật frame từ camera"""
        if not self.cap or not self.cap.isOpened():
            return

        ret, frame = self.cap.read()
        if not ret:
            self.cam_label.setText("Mất kết nối camera!")
            return

        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb.shape
        bytes_per_line = ch * w
        qimg = QImage(rgb.data, w, h, bytes_per_line, QImage.Format_RGB888)
        pixmap = QPixmap.fromImage(qimg).scaled(400, 300, Qt.KeepAspectRatio)
        self.cam_label.setPixmap(pixmap)

    def capture_from_stream(self):
        """Chụp 1 frame từ camera làm ảnh gốc"""
        if not self.cap or not self.cap.isOpened():
            self.status_label.setText("Camera chưa sẵn sàng!")
            return

        ret, frame = self.cap.read()
        if not ret:
            self.status_label.setText("Không chụp được ảnh!")
            return

        _, buf = cv2.imencode(".jpg", frame)
        self.image_bytes = base64.b64encode(buf).decode()
        self.image_path = None

        self.show_image(self.original_label, frame)
        self.status_label.setText("Đã capture! Nhấn Send để gửi.")

    def load_image(self):
        """Load ảnh từ file"""
        path, _ = QFileDialog.getOpenFileName(self, "Chọn ảnh", "", "Images (*.png *.jpg *.bmp)")
        if not path:
            return

        img = cv2.imread(path)
        if img is None:
            self.status_label.setText("Lỗi đọc ảnh!")
            return

        _, buf = cv2.imencode(".jpg", img)
        self.image_bytes = base64.b64encode(buf).decode()
        self.image_path = path

        self.show_image(self.original_label, img)
        self.status_label.setText("Ảnh đã load từ folder.")

    def send_image(self):
        """Gửi ảnh đến DE1-SOC và nhận kết quả"""
        if not self.image_bytes:
            self.status_label.setText("Chưa có ảnh để gửi!")
            return

        self.status_label.setText("Đang gửi ảnh...")

        try:
   
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(10)
                s.connect((SERVER_IP, PORT))
                img_data = base64.b64decode(self.image_bytes)
                s.sendall(img_data)
            self.status_label.setText("Đã gửi, đang chờ phản hồi...")
        except Exception as e:
            self.status_label.setText(f"Lỗi gửi: {e}")
            return

        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(15)
                s.bind(("0.0.0.0", PORT))
                s.listen(1)
                conn, addr = s.accept()
                data = b""
                with conn:
                    while True:
                        chunk = conn.recv(BUFFER_SIZE)
                        if not chunk:
                            break
                        data += chunk

            img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
            if img is not None:
                self.show_image(self.result_label, img)
                self.status_label.setText("Giải mã thành công!")
            else:
                self.status_label.setText("Lỗi giải mã ảnh!")
        except Exception as e:
            self.status_label.setText(f"Lỗi nhận: {e}")

    def show_image(self, label, img):
        """Hiển thị ảnh OpenCV lên QLabel"""
        h, w, ch = img.shape
        bytes_per_line = ch * w
        qimg = QImage(img.data, w, h, bytes_per_line, QImage.Format_BGR888)
        pixmap = QPixmap.fromImage(qimg).scaled(300, 300, Qt.KeepAspectRatio)
        label.setPixmap(pixmap)

    def closeEvent(self, event):
        """Dọn dẹp khi đóng"""
        if self.cap:
            self.cap.release()
        cv2.destroyAllWindows()
        super().closeEvent(event)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = App()
    window.show()
    sys.exit(app.exec_())
