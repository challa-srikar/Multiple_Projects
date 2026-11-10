#include "defines.h"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

char ssid[] = "Vivo";
char pass[] = "REDAPPLE";

BlynkTimer timer;
WidgetRTC rtc;

Servo servos[5];
int servoPins[5] = {13, 14, 12, 27, 26};
int buzzerPin = 25;

String medNames[5] = {"", "", "", "", ""};
int schedules[5] = {0, 0, 0, 0, 0};

int timeToMinutes(int h, int m) {
  return h * 60 + m;
}

// --- Time Inputs ---
BLYNK_WRITE(V10) {
  TimeInputParam t(param);
  schedules[0] = timeToMinutes(t.getStartHour(), t.getStartMinute());
  Serial.print("Chamber 1 schedule set to: ");
  Serial.println(schedules[0]);
}
BLYNK_WRITE(V11) {
  TimeInputParam t(param);
  schedules[1] = timeToMinutes(t.getStartHour(), t.getStartMinute());
  Serial.print("Chamber 2 schedule set to: ");
  Serial.println(schedules[1]);
}
BLYNK_WRITE(V12) {
  TimeInputParam t(param);
  schedules[2] = timeToMinutes(t.getStartHour(), t.getStartMinute());
  Serial.print("Chamber 3 schedule set to: ");
  Serial.println(schedules[2]);
}
BLYNK_WRITE(V13) {
  TimeInputParam t(param);
  schedules[3] = timeToMinutes(t.getStartHour(), t.getStartMinute());
  Serial.print("Chamber 4 schedule set to: ");
  Serial.println(schedules[3]);
}
BLYNK_WRITE(V14) {
  TimeInputParam t(param);
  schedules[4] = timeToMinutes(t.getStartHour(), t.getStartMinute());
  Serial.print("Chamber 5 schedule set to: ");
  Serial.println(schedules[4]);
}
// Manual Test Buttons for Each Chamber
BLYNK_WRITE(V20) { dispense(0); }
BLYNK_WRITE(V21) { dispense(1); }
BLYNK_WRITE(V22) { dispense(2); }
BLYNK_WRITE(V23) { dispense(3); }
BLYNK_WRITE(V24) { dispense(4); }

// --- Medicine Names ---
BLYNK_WRITE(V1) { medNames[0] = param.asStr(); Serial.println("Med name 1: " + medNames[0]); }
BLYNK_WRITE(V2) { medNames[1] = param.asStr(); Serial.println("Med name 2: " + medNames[1]); }
BLYNK_WRITE(V3) { medNames[2] = param.asStr(); Serial.println("Med name 3: " + medNames[2]); }
BLYNK_WRITE(V4) { medNames[3] = param.asStr(); Serial.println("Med name 4: " + medNames[3]); }
BLYNK_WRITE(V5) { medNames[4] = param.asStr(); Serial.println("Med name 5: " + medNames[4]); }

void dispense(int chamber) {
  Serial.print("Dispensing chamber ");
  Serial.println(chamber + 1);

  servos[chamber].write(45);
  digitalWrite(buzzerPin, HIGH);
  delay(50);
  servos[chamber].write(0);
  digitalWrite(buzzerPin, LOW);

  String msg = "Time to take: " + medNames[chamber];
  Blynk.logEvent("medicine_dispense", msg);
  Serial.println("Notification sent: " + msg);
}

void checkSchedule() {
  int nowMinutes = hour() * 60 + minute();
  Serial.print("Current time in minutes: ");
  Serial.println(nowMinutes);

  for (int i = 0; i < 5; i++) {
    Serial.print("Checking chamber ");
    Serial.print(i + 1);
    Serial.print(" - Scheduled at: ");
    Serial.println(schedules[i]);

    if (nowMinutes == schedules[i]) {
      Serial.println("Time match! Dispensing now...");
      dispense(i);
    }
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    dotCount++;
    if (dotCount > 20) {
      Serial.println("\nWiFi connection failed. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting Smart Medicine Dispenser...");

  connectToWiFi();

  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  for (int i = 0; i < 5; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(0);
    Serial.print("Servo ");
    Serial.print(i + 1);
    Serial.println(" initialized.");
  }

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  Serial.println("Buzzer initialized.");

  rtc.begin();
  Serial.println("RTC started.");

  timer.setInterval(60000L, checkSchedule);
  Serial.println("Timer set to check schedule every minute.");
}

void loop() {
  Blynk.run();
  timer.run();
}
