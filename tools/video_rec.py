import cv2
import socket
import numpy as np
from flask import Flask, Response

sock: socket.socket

app = Flask(__name__)

def udp_video_stream():
    buffer = b""
    while True:
        packet, _ = sock.recvfrom(4096)
        buffer += packet

        if len(buffer) > 64000:  # Prevent overflow
            buffer = b""

        if buffer.startswith(b"\xff\xd8") and buffer.endswith(b"\xff\xd9"):
            frame = np.frombuffer(buffer, dtype=np.uint8)
            img = cv2.imdecode(frame, cv2.IMREAD_COLOR)

            if img is not None:
                _, jpeg = cv2.imencode(".jpg", img)
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n\r\n')
                buffer = b""  # Reset buffer

@app.route('/video')
def video_feed():   
    return Response(udp_video_stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == "__main__":
    UDP_IP = "127.0.0.1"
    UDP_PORT = 9095
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((UDP_IP, UDP_PORT))
    app.run(host="0.0.0.0", port=8080, debug=False)
