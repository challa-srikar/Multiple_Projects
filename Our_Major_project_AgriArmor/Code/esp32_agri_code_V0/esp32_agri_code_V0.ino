#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "esp_http_server.h"

// ================= WIFI =================
const char* ssid = "FarmProtect";
const char* password = "12345678";

// ================= SERVER =================
WebServer server(80);

// ================= HTML =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Smart Farm</title>
  <style>
    body { text-align:center; background:#111; color:white; font-family:Arial; }
    button { padding:15px; margin:10px; font-size:18px; }
  </style>
</head>
<body>

<h1>🌱 Smart Crop Protection</h1>

<h2>Live Camera</h2>
<img src="http://192.168.4.1:81/stream" width="300">

<h2>Camera Control</h2>
<button onclick="fetch('/left')">⬅ LEFT</button>
<button onclick="fetch('/right')">➡ RIGHT</button>

<h2>Sprinkler</h2>
<button onclick="fetch('/pump/on')">ON</button>
<button onclick="fetch('/pump/off')">OFF</button>

<h2>UV Light</h2>
<button onclick="fetch('/uv/on')">ON</button>
<button onclick="fetch('/uv/off')">OFF</button>

</body>
</html>
)rawliteral";

// ================= PINS =================
#define RADAR_SERVO 2
#define CAM_SERVO 4
#define TRIG 12
#define ECHO 13
#define RELAY1 14
#define RELAY2 15
#define BUZZER 0

Servo radarServo;
Servo camServo;

int camAngle = 90;

// Radar control variables (NON-BLOCKING)
int angle = 0;
bool forward = true;
unsigned long lastMove = 0;

// ================= CAMERA =================
void startCamera() {
  Serial.println("[INFO] Initializing Camera...");

  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] Camera init failed: 0x%x\n", err);
    return;
  }

  Serial.println("[SUCCESS] Camera initialized");
}

// ================= STREAM =================
void startCameraServer(){
  Serial.println("[INFO] Starting Camera Stream Server...");

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = [](httpd_req_t *req){
      camera_fb_t * fb = NULL;
      char part_buf[64];

      httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");

      Serial.println("[STREAM] Client connected");

      while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("[ERROR] Camera capture failed");
          continue;
        }

        size_t hlen = snprintf(part_buf, 64,
          "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);

        httpd_resp_send_chunk(req, part_buf, hlen);
        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        httpd_resp_send_chunk(req, "\r\n", 2);

        esp_camera_fb_return(fb);
      }
      return ESP_OK;
    }
  };

  httpd_handle_t stream_httpd = NULL;
  httpd_start(&stream_httpd, &config);
  httpd_register_uri_handler(stream_httpd, &stream_uri);

  Serial.println("[SUCCESS] Stream started at port 81");
}

// ================= ULTRASONIC =================
long readDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH);
  long distance = duration * 0.034 / 2;

  return distance;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial.println("\n===== SYSTEM START =====");

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Serial.println("[INFO] Pins configured");

  radarServo.attach(RADAR_SERVO);
  camServo.attach(CAM_SERVO);

  Serial.println("[INFO] Servos attached");

  startCamera();

  Serial.println("[INFO] Starting WiFi AP...");
  WiFi.softAP(ssid, password);

  Serial.print("[INFO] IP Address: ");
  Serial.println(WiFi.softAPIP());

  startCameraServer();

  // Dashboard
  server.on("/", [](){
    Serial.println("[WEB] Dashboard opened");
    server.send_P(200, "text/html", index_html);
  });

  // Controls
  server.on("/left", [](){
    camAngle -= 10;
    camServo.write(camAngle);
    Serial.println("[ACTION] Camera LEFT");
    server.send(200, "text/plain", "LEFT");
  });

  server.on("/right", [](){
    camAngle += 10;
    camServo.write(camAngle);
    Serial.println("[ACTION] Camera RIGHT");
    server.send(200, "text/plain", "RIGHT");
  });

  server.on("/pump/on", [](){
    digitalWrite(RELAY1, HIGH);
    Serial.println("[ACTION] Pump ON");
    server.send(200, "text/plain", "Pump ON");
  });

  server.on("/pump/off", [](){
    digitalWrite(RELAY1, LOW);
    Serial.println("[ACTION] Pump OFF");
    server.send(200, "text/plain", "Pump OFF");
  });

  server.on("/uv/on", [](){
    digitalWrite(RELAY2, HIGH);
    Serial.println("[ACTION] UV ON");
    server.send(200, "text/plain", "UV ON");
  });

  server.on("/uv/off", [](){
    digitalWrite(RELAY2, LOW);
    Serial.println("[ACTION] UV OFF");
    server.send(200, "text/plain", "UV OFF");
  });

  server.begin();
  Serial.println("[SUCCESS] Web server started");
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  // Non-blocking radar scan
  if (millis() - lastMove > 150) {
    lastMove = millis();

    radarServo.write(angle);

    Serial.print("[SCAN] Angle: ");
    Serial.println(angle);

    long d = readDistance();

    Serial.print("[DISTANCE] ");
    Serial.print(d);
    Serial.println(" cm");

    if (d > 0 && d < 50) {
      Serial.println("[ALERT] Object detected!");

      camServo.write(angle);

      Serial.print("[CAM] Moved to angle: ");
      Serial.println(angle);

      digitalWrite(BUZZER, HIGH);
      Serial.println("[BUZZER] ON");
      delay(200);
      digitalWrite(BUZZER, LOW);
      Serial.println("[BUZZER] OFF");
    }

    // Sweep logic
    if (forward) {
      angle += 10;
      if (angle >= 180) forward = false;
    } else {
      angle -= 10;
      if (angle <= 0) forward = true;
    }
  }
}