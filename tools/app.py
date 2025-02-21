#!/usr/bin/env python3

import cv2
import socket
import numpy as np
from flask import Flask, Response, render_template, jsonify
from flask_cors import CORS
import logging

from telemetry_server import TelemetryServer
from threading import Thread

UDP_IP = "0.0.0.0"
UDP_PORT = 9095
TELEMETRY_IP = '192.168.0.202'
TELEMETRY_PORT = 9096

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
telemetry_server = TelemetryServer()
app = Flask(__name__)
CORS(app)

# ✅ Suppress logging for the /telemetry route
log = logging.getLogger("werkzeug")
log.setLevel(logging.WARNING)  # Only show warnings & errors


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

@app.route('/telemetry', )
def telemetry():
    log.setLevel(logging.ERROR)  # ✅ Hide normal logs, only show errors
    new_packets = telemetry_server.get_all_packets_no_block()
    return jsonify([packet.to_json() for packet in new_packets])

@app.route('/')
def index():
    return render_template('index.html')

if __name__ == "__main__":
    print(f'Opening UDP socket on {UDP_IP}:{UDP_PORT}')
    sock.bind((UDP_IP, UDP_PORT))

    Thread(
        target=telemetry_server.start,
        name='Telemtry_Server',
        args=(TELEMETRY_IP, TELEMETRY_PORT),
        daemon=True
    ).start()

    app.run(host="0.0.0.0", port=8080, debug=False)
