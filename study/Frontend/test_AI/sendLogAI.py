from flask import Flask, request, jsonify
import cv2
import base64
import numpy as np
from ultralytics import YOLO  # YOLOv8 사용 시

app = Flask(__name__)
model = YOLO('yolov8n.pt')  # 경량 모델로 예시

def decode_image(b64_string):
    data = base64.b64decode(b64_string.split(',')[-1])
    nparr = np.frombuffer(data, np.uint8)
    return cv2.imdecode(nparr, cv2.IMREAD_COLOR)

@app.route('/api/detect', methods=['POST'])
def detect():
    try:
        data = request.get_json()
        img = decode_image(data['image'])

        results = model(img)[0]
        detections = []
        for box in results.boxes:
            conf = float(box.conf)
            cls = int(box.cls)
            name = model.names[cls]
            detections.append({
                'class': name,
                'confidence': round(conf, 2)  # 소수점 2자리까지
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
