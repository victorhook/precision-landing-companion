#!/usr/bin/env python3

import cv2
import socket
import numpy as np
from flask import Flask, Response, render_template, jsonify, request
from flask_cors import CORS
import logging
import time

from telemetry_server import TelemetryServer, Action, TelemetryCommandAction, TelemetryPacketSubscriber, TelemetryTags, TelemetryPacketType, TelemetryCommandSetDetectionParams
from threading import Thread, Lock

UDP_IP = "0.0.0.0"
UDP_PORT = 9095
#TELEMETRY_IP = '192.168.4.1'
#TELEMETRY_IP = '192.168.0.202'
TELEMETRY_IP = '127.0.0.1'
TELEMETRY_PORT = 9096

last_status_request = 0

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
CORS(app, resources={r"/*": {"origins": "*"}}, supports_credentials=True)

# ✅ Suppress logging for the /telemetry route
log = logging.getLogger("werkzeug")
log.setLevel(logging.WARNING)  # Only show warnings & errors


def udp_video_stream():
    buffer = b""

    def draw_tag_rectangles(img):
        # ✅ Get detected tags from telemetry
        global tag_handler
        detected_tags = tag_handler.get_tags()  # Replace with your actual function
        if detected_tags is None:
            return

        for tag in detected_tags.tags:
            # ✅ Extract coordinates from `Point2F`
            x_coords = [tag.p1.x, tag.p2.x, tag.p3.x, tag.p4.x]
            y_coords = [tag.p1.y, tag.p2.y, tag.p3.y, tag.p4.y]

            # ✅ Find top-left (min X, min Y) and bottom-right (max X, max Y)
            pt1 = (int(min(x_coords)), int(min(y_coords)))  # Top-left
            pt2 = (int(max(x_coords)), int(max(y_coords)))  # Bottom-right

            # ✅ Draw rectangle
            if detected_tags.has_lock and tag.id == detected_tags.locked_tag.id:
                color = (0, 255, 0)
            else:
                color = (0, 0, 150)

            cv2.rectangle(img, pt1, pt2, color, 2)


    def draw_text(img):
        global fps, tag_handler
        detected_tags = tag_handler.get_tags()  # Replace with your actual function

        cv2.putText(img, f"FPS: {fps}", (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        
        if detected_tags is None:
            return

        if detected_tags.has_lock:
            color = (0, 255, 0)
            cv2.putText(img, f"Ax: {round(detected_tags.landing_target.angle_x, 1)}", (10, 40),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            cv2.putText(img, f"Ay: {round(detected_tags.landing_target.angle_y, 1)}", (10, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            cv2.putText(img, f"SX: {round(detected_tags.landing_target.size_x, 1)}", (10, 80),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            cv2.putText(img, f"Sy: {round(detected_tags.landing_target.size_y, 1)}", (10, 100),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            cv2.putText(img, f"D: {round(detected_tags.landing_target.distance, 1)}", (10, 120),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)

        else:
            color = (0, 0, 255)
        cv2.putText(img, f"Lock", (270, 20),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

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

def response_success(msg: str) -> Response:
    return jsonify({"status": "success", "message": msg}), 200

def response_error(msg: str) -> Response:
    return jsonify({"status": "error", "message": msg}), 400


@app.route('/command', methods=['POST'])
def command():
    try:
        data = request.get_json()
        actions = {
            'Arm': Action.ARM,
            'Disarm': Action.DISARM,
            'TakeOff': Action.TAKEOFF,
            'Land': Action.LAND,
            'ArmCheck': Action.ARM_CHECK,
            'Reboot': Action.REBOOT,
            'Reboot AP': Action.REBOOT_AP,
        }
        action = actions.get(data)
        if action is None:
            print(f'Invalid command {data}')
            return response_error('Invalid command')
        
        print(f'Command: {data} -> {action}')
        telemetry_server.send_packet(TelemetryCommandAction(action))
        return response_success('Command sent')
    except Exception as e:
        return response_error(str(e))

@app.route('/telemetry')
def telemetry():
    global last_status_request
    last_status_request = time.time()
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
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((UDP_IP, UDP_PORT))

    Thread(
        target=telemetry_server.start,
        name='Telemtry_Server',
        args=(TELEMETRY_IP, TELEMETRY_PORT),
        daemon=True
    ).start()

    port = 8080
    print(f'App port: {port}')
    app.run(host="0.0.0.0", port=port, debug=False)
