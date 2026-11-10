import cv2
from controller import doorAutomate
import time
import threading
from flask import Flask, request

# ---------------- FLASK SERVER ----------------

app = Flask(__name__)

def unlock_door():
    print("Unlocking Door...")
    doorAutomate(0)  # Unlock
    time.sleep(10)
    doorAutomate(1)  # Lock
    print("Door Locked")

@app.route('/unlock')
def unlock():
    key = request.args.get("key")
    if key == "funt123":   # Change this password
        threading.Thread(target=unlock_door).start()
        return "Door Unlocked"
    else:
        return "Access Denied"

def run_server():
    app.run(host='0.0.0.0', port=5000)

# Run Flask in background
threading.Thread(target=run_server, daemon=True).start()

# ---------------- FACE RECOGNITION ----------------

video = cv2.VideoCapture(0)

facedetect = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")

recognizer = cv2.face.LBPHFaceRecognizer_create()
recognizer.read("Trainer.yml")

name_list = ["", "Srikar", "Govind"]

imgBackground = cv2.imread("background.png")

conf = None

while True:
    ret, frame = video.read()
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    gray = cv2.equalizeHist(gray)

    faces = facedetect.detectMultiScale(gray, 1.3, 5)

    for (x, y, w, h) in faces:

        face = gray[y:y+h, x:x+w]
        face = cv2.resize(face, (200, 200))

        serial, conf = recognizer.predict(face)
        print("Confidence:", conf)

        if conf < 55:
            name = name_list[serial]
            color = (0, 255, 0)
        else:
            name = "Unknown"
            color = (0, 0, 255)

        cv2.rectangle(frame, (x, y), (x+w, y+h), color, 2)
        cv2.rectangle(frame, (x, y-40), (x+w, y), color, -1)
        cv2.putText(frame, name, (x, y-10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8,
                    (255, 255, 255), 2)

    frame = cv2.resize(frame, (640, 480))
    imgBackground[162:162 + 480, 55:55 + 640] = frame
    cv2.imshow("Frame", imgBackground)

    k = cv2.waitKey(1)

    # Keyboard unlock
    if k == ord('o') and conf is not None and conf < 55:
        unlock_door()

    if k == ord('q'):
        break

video.release()
cv2.destroyAllWindows()