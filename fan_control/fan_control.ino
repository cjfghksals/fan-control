#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

const char* WIFI_SSID      = "KT_GiGA_16C3";
const char* WIFI_PASSWORD  = "0fb90dh360";
const char* MQTT_HOST      = "f228bba902564c449137624b48611c35.s1.eu.hivemq.cloud";
const int   MQTT_PORT      = 8883;
const char* MQTT_USER      = "cjfghksals";
const char* MQTT_PASSWORD  = "Ehdals6469!";
const char* TOPIC_CONTROL  = "fan/control";
const char* TOPIC_STATUS   = "fan/status";
const char* TOPIC_TIMER    = "fan/timer";
const char* TOPIC_TEMP     = "fan/temp";
const char* TOPIC_PRESSURE = "fan/pressure";
const char* TOPIC_TEMPMODE = "fan/tempmode";

#define FAN_PIN D1
#define SDA_PIN D2
#define SCL_PIN D5

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_BMP085 bmp;

bool  timerActive    = false;
unsigned long timerRemain = 0;
unsigned long lastSecond  = 0;
unsigned long lastSensor  = 0;

bool  tempModeActive  = false;
float activationTemp  = 0;
float targetTemp      = 0;

void setup() {
  Serial.begin(115200);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bmp.begin()) {
    Serial.println("\nBMP180 센서를 찾을 수 없어요.\n");
  } else {
    Serial.println("\nBMP180 초기화 완료\n");
  }

  connectWiFi();
  wifiClient.setInsecure();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
}

void loop() {
  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  // 타이머
  if (timerActive && millis() - lastSecond >= 1000) {
    lastSecond = millis();
    if (timerRemain > 0) {
      timerRemain--;
      mqtt.publish(TOPIC_TIMER, String(timerRemain).c_str(), true);
    }
    if (timerRemain == 0) {
      digitalWrite(FAN_PIN, LOW);
      timerActive = false;
      mqtt.publish(TOPIC_STATUS, "0", true);
      mqtt.publish(TOPIC_TIMER,  "0", true);
      Serial.println("\n타이머 종료 - 팬 OFF\n");
    }
  }

  // 센서 10초마다 측정
  if (millis() - lastSensor >= 10000) {
    lastSensor = millis();
    publishSensor();
  }
}

void publishSensor() {
  float temp  = bmp.readTemperature();
  float press = bmp.readPressure() / 100.0;
  mqtt.publish(TOPIC_TEMP,     String(temp, 1).c_str(), true);
  mqtt.publish(TOPIC_PRESSURE, String(press, 1).c_str(), true);
  Serial.println("온도: " + String(temp, 1) + "°C  기압: " + String(press, 1) + "hPa\n");

  // 목표 온도 모드
  if (tempModeActive) {
    bool fanOn = digitalRead(FAN_PIN);
    if (temp >= activationTemp && !fanOn) {
      digitalWrite(FAN_PIN, HIGH);
      mqtt.publish(TOPIC_STATUS, "1", true);
      Serial.println("온도 모드 - 팬 ON (현재: " + String(temp, 1) + "°C)\n");
    } else if (temp <= targetTemp && fanOn) {
      digitalWrite(FAN_PIN, LOW);
      mqtt.publish(TOPIC_STATUS, "0", true);
      Serial.println("온도 모드 - 팬 OFF (현재: " + String(temp, 1) + "°C)\n");
    }
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("수신: " + msg + "\n");

  if (msg == "1") {
    digitalWrite(FAN_PIN, HIGH);
    timerActive = false;
    mqtt.publish(TOPIC_STATUS, "1", true);
    mqtt.publish(TOPIC_TIMER,  "0", true);
  } else if (msg == "0") {
    digitalWrite(FAN_PIN, LOW);
    timerActive = false;
    mqtt.publish(TOPIC_STATUS, "0", true);
    mqtt.publish(TOPIC_TIMER,  "0", true);
  } else if (msg.startsWith("T")) {
    timerRemain = msg.substring(1).toInt();
    if (timerRemain > 0) {
      tempModeActive = false;
      mqtt.publish(TOPIC_TEMPMODE, "0", true);
      digitalWrite(FAN_PIN, HIGH);
      timerActive = true;
      lastSecond  = millis();
      mqtt.publish(TOPIC_STATUS, "1", true);
      mqtt.publish(TOPIC_TIMER,  String(timerRemain).c_str(), true);
    }
  } else if (msg.startsWith("TEMP:")) {
    int f = msg.indexOf(':');
    int s = msg.indexOf(':', f + 1);
    activationTemp = msg.substring(f + 1, s).toFloat();
    targetTemp     = msg.substring(s + 1).toFloat();
    tempModeActive = true;
    timerActive    = false;
    mqtt.publish(TOPIC_TIMER, "0", true);
    String status = "1:" + String(activationTemp, 1) + ":" + String(targetTemp, 1);
    mqtt.publish(TOPIC_TEMPMODE, status.c_str(), true);
    Serial.println("온도 모드 시작 - 가동: " + String(activationTemp, 1) + "°C  목표: " + String(targetTemp, 1) + "°C\n");
  } else if (msg == "TEMPSTOP") {
    tempModeActive = false;
    mqtt.publish(TOPIC_TEMPMODE, "0", true);
    Serial.println("온도 모드 종료\n");
  }
}

void connectWiFi() {
  Serial.println("\nWiFi 연결 중...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi 연결됨: " + WiFi.localIP().toString() + "\n");
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    Serial.println("\nMQTT 연결 중...");
    if (mqtt.connect("NodeMCU-Fan", MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("MQTT 연결됨\n");
      mqtt.subscribe(TOPIC_CONTROL);
      publishSensor();
    } else {
      Serial.println("MQTT 실패: " + String(mqtt.state()) + "\n");
      delay(3000);
    }
  }
}
