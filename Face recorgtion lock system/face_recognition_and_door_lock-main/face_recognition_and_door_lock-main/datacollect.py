import cv2
import os

video = cv2.VideoCapture(0)  # Laptop camera

if not video.isOpened():
    print("Camera not working!")
    exit()

facedetect = cv2.CascadeClassifier("haarcascade_frontalface_default.xml")

id = input("Enter Your ID: ")
count = 0

if not os.path.exists("datasets"):
    os.makedirs("datasets")

while True:
    ret, frame = video.read()

    if not ret:
        print("Failed to grab frame")
        break

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    faces = facedetect.detectMultiScale(gray, 1.3, 5)

    for (x, y, w, h) in faces:
        count += 1
        cv2.imwrite(f'datasets/User.{id}.{count}.jpg', gray[y:y+h, x:x+w])
        cv2.rectangle(frame, (x, y), (x+w, y+h), (50,50,255), 2)

    cv2.imshow("Frame", frame)

    if cv2.waitKey(1) == 27:  # Press ESC to exit
        break

    if count > 300:
        break

video.release()
cv2.destroyAllWindows()

print("Dataset Collection Done")