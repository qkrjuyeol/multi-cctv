from flask import Flask, request, jsonify
import cv2
import base64
import numpy as np
from ultralytics import YOLO
from datetime import datetime
import os
import csv

app = Flask(__name__)
model = YOLO('yolov8n.pt')  # 경량 모델 사용

LOG_DIR = 'logs'  # 로그 저장 폴더

# 로그 저장 함수
def log_event(camera_id, event):
    now = datetime.now()
    date_str = now.strftime('%Y-%m-%d')
    time_str = now.strftime('%H:%M:%S')
    
    # 로그 파일 경로
    os.makedirs(LOG_DIR, exist_ok=True)
    log_file_path = os.path.join(LOG_DIR, f"{date_str}.csv")

    file_exists = os.path.isfile(log_file_path)
    with open(log_file_path, 'a', newline='') as f:
        writer = csv.writer(f)
        if not file_exists:
            writer.writerow(['time', 'camera_id', 'event'])  # 헤더
        writer.writerow([time_str, camera_id, event])

# 이미지 디코딩 함수
def decode_image(b64_string):
    data = base64.b64decode(b64_string.split(',')[-1])
    nparr = np.frombuffer(data, np.uint8)
    return cv2.imdecode(nparr, cv2.IMREAD_COLOR)

@app.route('/api/detect', methods=['POST'])
def detect():
    try:
        data = request.get_json()
        img = decode_image(data['image'])
        camera_id = data.get('camera_id', 'unknown')  # MFC 측에서 camera_id 포함 필요

        results = model(img)[0]
        detections = []
        for box in results.boxes:
            conf = float(box.conf)
            cls = int(box.cls)
            name = model.names[cls]
            event = f"{name} detected"
            log_event(camera_id, event)  # 로그 저장
            detections.append({
                'class': name,
                'confidence': round(conf, 2)
            })

        if not detections:
            return jsonify({'status': 'no_object'}), 204

        return jsonify({
            'status': 'ok',
            'detections': detections
        })

    except Exception as e:
        return jsonify({'status': 'error', 'message': str(e)}), 500

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
