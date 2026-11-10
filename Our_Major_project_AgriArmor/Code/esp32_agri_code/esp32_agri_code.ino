#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "esp_http_server.h"
#include "time.h"
#include <WiFiClientSecure.h>

// WIFI
const char* ssid = "ACT_6A55";
const char* password = "vfmGBmQ5";

WebServer server(80);

String BOT_TOKEN = "8669601885:AAFOCF1OqTQIc-s_FcuykEIwy4mZlpynJf0";
String CHAT_ID = "5032480483";
WiFiClientSecure client;

// QUEUE SYSTEM
bool sendPhotoFlag = false;
unsigned long lastTelegramTime = 0;

// NTP
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;

// ================= SCHEDULE =================
int pumpStartH=-1, pumpStartM=-1, pumpStopH=-1, pumpStopM=-1;
int uvStartH=-1, uvStartM=-1, uvStopH=-1, uvStopM=-1;

// ================= UV MANUAL =================
bool uvManualOverride = false;
bool uvManualState = false;

// ================= MOTION HOLD =================
unsigned long lastMotionTime = 0;
const unsigned long motionHoldTime = 30000;

// ================= PINS =================
#define RADAR_SERVO 2
#define CAM_SERVO 4
#define TRIG 12
#define ECHO 13
#define RELAY1 14
#define RELAY2 15

Servo radarServo, camServo;

int angle=0;
bool forward=true;
unsigned long lastMove=0;

// ================= TELEGRAM =================
void sendTelegramMessage(String msg){
  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) return;

  String url = "/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + msg;

  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Connection: close");
  client.println();
}

void sendPhotoTelegram(){
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    return;
  }

  client.setInsecure();
  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("Telegram connection failed");
    esp_camera_fb_return(fb);
    return;
  }

  String boundary = "----WebKitFormBoundary";
  String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + CHAT_ID + "\r\n";
  head += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";

  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();

  client.println("POST /bot" + BOT_TOKEN + "/sendPhoto HTTP/1.1");
  client.println("Host: api.telegram.org");
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(totalLen));
  client.println("Connection: close");
  client.println();

  client.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;

  for (size_t i = 0; i < fbLen; i += 1024) {
    if (i + 1024 < fbLen)
      client.write(fbBuf + i, 1024);
    else
      client.write(fbBuf + i, fbLen - i);
  }

  client.print(tail);

  esp_camera_fb_return(fb);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  Serial.println("Photo sent successfully");
}

// ================= HTML =================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body style="background:#111;color:white;text-align:center;">
<h1>Smart Farm</h1>

<img src="/stream" 
     onerror="this.src='http://'+location.hostname+':81/stream'" 
     width="400"
     style="transform: rotate(180deg);">

<h2>Manual</h2>
<button onclick="fetch('/pump/on')">Pump ON</button>
<button onclick="fetch('/pump/off')">Pump OFF</button><br>
<button onclick="fetch('/uv/on')">UV ON</button>
<button onclick="fetch('/uv/off')">UV OFF</button>
<button onclick="fetch('/uv/auto')">UV AUTO</button>

<h2>Pump Schedule</h2>
Start H:<input id="psh"> M:<input id="psm"><br>
Stop H:<input id="peh"> M:<input id="pem"><br>
<button onclick="setPump()">Set</button>

<h2>UV Schedule</h2>
Start H:<input id="ush"> M:<input id="usm"><br>
Stop H:<input id="ueh"> M:<input id="uem"><br>
<button onclick="setUV()">Set</button>

<script>
function setPump(){
fetch(`/pump/set?sh=${psh.value}&sm=${psm.value}&eh=${peh.value}&em=${pem.value}`);
}
function setUV(){
fetch(`/uv/set?sh=${ush.value}&sm=${usm.value}&eh=${ueh.value}&em=${uem.value}`);
}
</script>
</body>
</html>
)rawliteral";

// ================= CAMERA =================
void startCamera(){
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = 5; config.pin_d1 = 18; config.pin_d2 = 19; config.pin_d3 = 21;
  config.pin_d4 = 36; config.pin_d5 = 39; config.pin_d6 = 34; config.pin_d7 = 35;

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

  esp_camera_init(&config);
}

// ================= STREAM =================
void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;

  httpd_uri_t stream_uri = {
    .uri="/stream",
    .method=HTTP_GET,
    .handler=[](httpd_req_t *req){

      camera_fb_t * fb=NULL;
      char buf[64];

      httpd_resp_set_type(req,"multipart/x-mixed-replace; boundary=frame");

      while(true){
        fb=esp_camera_fb_get();
        if(!fb) continue;

        size_t hlen=snprintf(buf,64,
        "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",fb->len);

        httpd_resp_send_chunk(req,buf,hlen);
        httpd_resp_send_chunk(req,(const char*)fb->buf,fb->len);
        httpd_resp_send_chunk(req,"\r\n",2);

        esp_camera_fb_return(fb);
      }
      return ESP_OK;
    }
  };

  httpd_handle_t stream_httpd=NULL;
  httpd_start(&stream_httpd,&config);
  httpd_register_uri_handler(stream_httpd,&stream_uri);
}

// ================= ULTRASONIC =================
long readDistance(){
  digitalWrite(TRIG,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG,HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG,LOW);
  long duration=pulseIn(ECHO,HIGH);
  return duration*0.034/2;
}

// ================= SETUP =================
void setup(){
  Serial.begin(115200);

  pinMode(TRIG,OUTPUT);
  pinMode(ECHO,INPUT);
  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);

  digitalWrite(RELAY1,HIGH);
  digitalWrite(RELAY2,HIGH);

  radarServo.attach(RADAR_SERVO);
  camServo.attach(CAM_SERVO);

  startCamera();

  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){ delay(500); }

  configTime(gmtOffset_sec,0,ntpServer);

  startCameraServer();

  server.on("/", [](){ server.send_P(200,"text/html",index_html); });

  server.on("/pump/on", [](){ digitalWrite(RELAY1,LOW); });
  server.on("/pump/off", [](){ digitalWrite(RELAY1,HIGH); });

  server.on("/uv/on", [](){
    uvManualOverride = true;
    uvManualState = true;
    digitalWrite(RELAY2,LOW);
  });

  server.on("/uv/off", [](){
    uvManualOverride = true;
    uvManualState = false;
    digitalWrite(RELAY2,HIGH);
  });

  server.on("/uv/auto", [](){
    uvManualOverride = false;
    lastMotionTime = millis() - motionHoldTime - 1;
    digitalWrite(RELAY2, HIGH);
  });

  server.on("/pump/set", [](){
    pumpStartH=server.arg("sh").toInt();
    pumpStartM=server.arg("sm").toInt();
    pumpStopH=server.arg("eh").toInt();
    pumpStopM=server.arg("em").toInt();
  });

  server.on("/uv/set", [](){
    uvStartH=server.arg("sh").toInt();
    uvStartM=server.arg("sm").toInt();
    uvStopH=server.arg("eh").toInt();
    uvStopM=server.arg("em").toInt();
  });

  server.begin();
}

// ================= LOOP =================
void loop(){
  server.handleClient();

  struct tm timeinfo;
  int h=0,m=0;

  if(getLocalTime(&timeinfo)){
    h=timeinfo.tm_hour;
    m=timeinfo.tm_min;
  }

  if(pumpStartH!=-1){
    if((h > pumpStartH || (h==pumpStartH && m>=pumpStartM)) &&
       (h < pumpStopH  || (h==pumpStopH  && m<pumpStopM))){
      digitalWrite(RELAY1,LOW);
    } else digitalWrite(RELAY1,HIGH);
  }

  if(millis()-lastMove>150){
    lastMove=millis();

    radarServo.write(angle);
    long d=readDistance();

    if(d>0 && d<50){
      lastMotionTime = millis();
      camServo.write(angle);

      if(millis() - lastTelegramTime > 15000){
        sendTelegramMessage("⚠ Motion Detected in Farm!");
        sendPhotoFlag = true;
        lastTelegramTime = millis();
      }
    }

    if(forward){ angle+=10; if(angle>=180) forward=false; }
    else{ angle-=10; if(angle<=0) forward=true; }
  }

  // ===== PHOTO QUEUE HANDLER =====
  if(sendPhotoFlag){
    Serial.println("Sending photo...");
    delay(500);
    sendPhotoTelegram();
    delay(1200);
    sendPhotoFlag = false;
    Serial.println("Photo sent done");
  }

  bool motionActive = (millis() - lastMotionTime < motionHoldTime);

  if(uvManualOverride){
    digitalWrite(RELAY2, uvManualState ? LOW : HIGH);
  } else {
    bool uvScheduleActive = false;

    if(uvStartH!=-1){
      if((h > uvStartH || (h==uvStartH && m>=uvStartM)) &&
         (h < uvStopH  || (h==uvStopH  && m<uvStopM))){
        uvScheduleActive = true;
      }
    }

    if(uvScheduleActive || motionActive){
      digitalWrite(RELAY2,LOW);
    } else {
      digitalWrite(RELAY2,HIGH);
    }
  }
}