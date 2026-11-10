#include <WiFi.h>
#include <PubSubClient.h>

#define IR1 14
#define IR2 27

#define MAX_CAPACITY 40
#define COOLDOWN 1500  // 1.5 seconds

const char* ssid = "ACT_6A55";
const char* password = "vfmGBmQ5";
const char* mqtt_server = "192.168.0.10";

WiFiClient espClient;
PubSubClient client(espClient);

int count = 0;

// -------- STATE --------
int state = 0;  // 0=idle, 1=IR1 first, 2=IR2 first
unsigned long lastTriggerTime = 0;
unsigned long lastCountTime = 0;


// -------- WIFI --------
void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}


// -------- MQTT --------
void reconnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");

    if (client.connect("ESP32_PASSENGER")) {
      Serial.println("MQTT Connected!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(2000);
    }
  }
}


// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("System Ready...");
}


// -------- LOOP --------
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  int s1 = digitalRead(IR1);
  int s2 = digitalRead(IR2);

  // 🔒 COOLDOWN: ignore everything
  if (now - lastCountTime < COOLDOWN) {
    return;
  }

  // 🚫 INVALID CASE: both sensors blocked
  if (s1 == LOW && s2 == LOW) {
    if (state != 0 && (now - lastTriggerTime > 300)) {
      Serial.println("❌ Invalid: Both sensors blocked");
      resetState();
    }
    return;
  }

  // -------- IDLE --------
  if (state == 0) {
    if (s1 == LOW) {
      state = 1;
      lastTriggerTime = now;
      Serial.println("IR1 first → ENTRY start");
    }
    else if (s2 == LOW) {
      state = 2;
      lastTriggerTime = now;
      Serial.println("IR2 first → EXIT start");
    }
  }

  // -------- ENTRY --------
  else if (state == 1) {

    // timeout protection
    if (now - lastTriggerTime > 1500) {
      Serial.println("⏱ ENTRY timeout");
      resetState();
      return;
    }

    if (s2 == LOW) {

      if (count < MAX_CAPACITY) {
        count++;
        Serial.println("ENTRY ✅");
      } else {
        Serial.println("Bus Full ❌");
      }

      publishCount();
      lastCountTime = now;
      resetState();
    }
  }

  // -------- EXIT --------
  else if (state == 2) {

    // timeout protection
    if (now - lastTriggerTime > 1500) {
      Serial.println("⏱ EXIT timeout");
      resetState();
      return;
    }

    if (s1 == LOW) {

      if (count > 0) {
        count--;
        Serial.println("EXIT ✅");
      } else {
        Serial.println("Already Empty ❌");
      }

      publishCount();
      lastCountTime = now;
      resetState();
    }
  }

  delay(50);
}


// -------- RESET --------
void resetState() {
  state = 0;
}


// -------- MQTT SEND --------
void publishCount() {

  // safety clamp
  if (count < 0) count = 0;
  if (count > MAX_CAPACITY) count = MAX_CAPACITY;

  String payload = String(count);

  Serial.print("Publishing Count: ");
  Serial.println(payload);

  if (client.publish("bus/BUS001/passenger", payload.c_str())) {
    Serial.println("Publish Success");
  } else {
    Serial.println("Publish Failed");
  }
}