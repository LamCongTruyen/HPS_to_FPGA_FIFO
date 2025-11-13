# client.py
import socket

SERVER_IP = "192.168.100.2"   # IP Linux VM (host-only)
PORT = 8080
FILE = "20250910012013.jpg"              # ảnh cần gửi

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((SERVER_IP, PORT))
    with open(FILE, "rb") as f:
        data = f.read()
        s.sendall(data)

print("Đã gửi xong ảnh.")
