# 🚌 Smart Bus Tracking & Passenger Monitoring System

A real-time IoT-based system for tracking buses, monitoring passenger count, and displaying live seat availability using:

* ESP32 + IR Sensors
* MQTT (Mosquitto)
* Flask + SocketIO
* Live Web Dashboard (Driver & Passenger)

---

# 🚀 Features

## 👤 Passenger Dashboard

* Live passenger count
* Seat availability
* Occupancy %
* Bus live tracking on map
* ETA for stops
* Traffic visualization

## 🚍 Driver Dashboard

* Live GPS tracking
* Add/remove stops
* Route optimization
* Manual GPS sync
* Animated bus movement

## 📡 IoT Features

* ESP32-based passenger counting
* MQTT communication
* Real-time updates (SocketIO)

---

# 🧠 System Architecture

ESP32 → MQTT → Mosquitto Broker → Flask Server → SocketIO → Web Dashboard

---

# 🖥️ Setup Instructions (Raspberry Pi)

## 1️⃣ Install System Dependencies

```bash
sudo apt update
sudo apt install -y python3-pip python3-venv mosquitto mosquitto-clients
```

---

## 2️⃣ Start MQTT Broker

```bash
sudo systemctl start mosquitto
sudo systemctl enable mosquitto
```

Check status:

```bash
sudo systemctl status mosquitto
```

---

## 3️⃣ Clone Project

```bash
git clone <your-repo-url>
cd <your-project-folder>
```

---

## 4️⃣ Create Virtual Environment

```bash
python3 -m venv venv
source venv/bin/activate
```

---

## 5️⃣ Install Python Dependencies

```bash
pip install -r requirements.txt
```

---

## 6️⃣ Run the Server

```bash
python3 server.py
```

Server will start at:

```
http://<raspberry-pi-ip>:5000
```

---

# 📱 Access Dashboards

* Passenger Dashboard:

```
http://<IP>:5000/
```

* Driver Dashboard:

```
http://<IP>:5000/driver
```

---

# 🔧 MQTT Test (Optional)

## Terminal 1:

```bash
mosquitto_sub -h localhost -t bus/BUS001/passenger
```

## Terminal 2:

```bash
mosquitto_pub -h localhost -t bus/BUS001/passenger -m "10"
```

---

# 🔌 ESP32 Setup

* Connect IR sensors to pins:

  * IR1 → GPIO 14
  * IR2 → GPIO 27

* Update WiFi + MQTT IP in code:

```cpp
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "RASPBERRY_PI_IP";
```

---

# 📦 requirements.txt

```
Flask==3.1.3
Flask-SocketIO==5.6.1
eventlet==0.40.4
paho-mqtt==2.1.0
geopy==2.4.1
requests==2.32.5
```

---

# ⚠️ Notes

* Ensure ESP32 and Raspberry Pi are on the same WiFi network
* Port 5000 should be accessible
* If SocketIO issues occur, ensure `eventlet` is installed
* For Raspberry Pi, use `pip3` if needed

---

# 🚀 Future Improvements

* Multi-bus tracking system
* Mobile app integration
* AI-based passenger counting (camera)
* Cloud deployment (AWS / Render)

---

# 👨‍💻 Author

Funt Robotics

---
