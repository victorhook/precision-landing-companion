#!/usr/bin/env python3

import cv2
import socket
import numpy as np
from flask import Flask, Response, render_template, jsonify, request
from flask_cors import CORS
import logging

from telemetry_server import TelemetryServer, TelemetryPacket, TelemetryPacketSubscriber, TelemetryTags, TelemetryPacketType, TelemetryCommandSetDetectionParams
from threading import Thread, Lock

UDP_IP = "0.0.0.0"
UDP_PORT = 9095
TELEMETRY_IP = '192.168.0.202'
TELEMETRY_PORT = 9096

class TagHandler(TelemetryPacketSubscriber):

    def __init__(self) -> None:
        self._tag_lock = Lock()
        self._tags: TelemetryTags = None

    def handle_telemetry_packet(self, packet: TelemetryTags) -> None:
        with self._tag_lock:
            self._tags = packet

    def get_tags(self) -> TelemetryTags:
        with self._tag_lock:
            return self._tags

fps = 0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
telemetry_server = TelemetryServer()
tag_handler = TagHandler()
telemetry_server.subscribe(TelemetryPacketType.TAGS, tag_handler)
app = Flask(__name__)
CORS(app)

# ✅ Suppress logging for the /telemetry route
log = logging.getLogger("werkzeug")
log.setLevel(logging.WARNING)  # Only show warnings & errors


def udp_video_stream():
    buffer = b""

    def draw_tag_rectangles(img):
        # ✅ Get detected tags from telemetry
        global tag_handler
        detected_tags = tag_handler.get_tags()  # Replace with your actual function

        for tag in detected_tags.tags:
            # ✅ Extract coordinates from `Point2F`
            x_coords = [tag.p1.x, tag.p2.x, tag.p3.x, tag.p4.x]
            y_coords = [tag.p1.y, tag.p2.y, tag.p3.y, tag.p4.y]

            # ✅ Find top-left (min X, min Y) and bottom-right (max X, max Y)
            pt1 = (int(min(x_coords)), int(min(y_coords)))  # Top-left
            pt2 = (int(max(x_coords)), int(max(y_coords)))  # Bottom-right

            # ✅ Draw rectangle
            cv2.rectangle(img, pt1, pt2, (0, 255, 0), 2)

            # ✅ Draw text with tag ID
            #cv2.putText(img, f"ID: {tag.id}", (pt1[0], pt1[1] - 10),
            #            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    def draw_text(img):
        global fps
        cv2.putText(img, f"FPS: {fps}", (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    while True:
        packet, _ = sock.recvfrom(4096)
        buffer += packet

        if len(buffer) > 64000:  # Prevent overflow
            buffer = b""

        if buffer.startswith(b"\xff\xd8") and buffer.endswith(b"\xff\xd9"):
            frame = np.frombuffer(buffer, dtype=np.uint8)
            img = cv2.imdecode(frame, cv2.IMREAD_COLOR)

            if img is not None:
                draw_tag_rectangles(img)

                draw_text(img)

                _, jpeg = cv2.imencode(".jpg", img)
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + jpeg.tobytes() + b'\r\n\r\n')
                buffer = b""  # Reset buffer

@app.route("/set_detection_params", methods=["POST"])
def set_detection_params():
    try:
        data = request.get_json()
        telemetry_server.send_packet(TelemetryCommandSetDetectionParams(**data))
        return jsonify({"status": "success", "message": "Parameters updated"}), 200
    except Exception as e:
        return jsonify({"status": "error", "message": str(e)}), 400

@app.route('/video')
def video_feed():   
    return Response(udp_video_stream(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/telemetry', )
def telemetry():
    log.setLevel(logging.ERROR)  # ✅ Hide normal logs, only show errors
    new_packets = telemetry_server.get_all_packets_no_block()
    for packet in new_packets:
        if hasattr(packet, 'cameraFps'):
            global fps
            fps = packet.cameraFps
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
