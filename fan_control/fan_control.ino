#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>

const char* WIFI_SSID       = "iptime";
const char* WIFI_PASSWORD   = "";
const char* MQTT_HOST       = "f228bba902564c449137624b48611c35.s1.eu.hivemq.cloud";
const int   MQTT_PORT       = 8883;
const char* MQTT_USER       = "cjfghksals";
const char* MQTT_PASSWORD   = "Ehdals6469!";
const char* TOPIC_CONTROL   = "fan/control";
const char* TOPIC_STATUS    = "fan/status";
const char* TOPIC_TIMER     = "fan/timer";
const char* TOPIC_TEMP      = "fan/temp";
const char* TOPIC_PRESSURE  = "fan/pressure";
const char* TOPIC_TEMP_CTL  = "fan/temp_control";
const char* TOPIC_TEMP_MODE = "fan/temp_mode";

#define FAN_PIN D1
#define SDA_PIN D2
#define SCL_PIN D5

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_BMP085 bmp;

bool  fanOn          = false;
bool  timerActive    = false;
bool  tempModeActive = false;
float triggerTemp    = 0;
float targetTemp     = 0;
unsigned long timerRemain = 0;
unsigned long lastSecond  = 0;
unsigned long lastSensor  = 0;

void setFan(bool on) {
  fanOn = on;
  digitalWrite(FAN_PIN, on ? HIGH : LOW);
  mqtt.publish(TOPIC_STATUS, on ? "1" : "0", true);
}

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

  if (timerActive && millis() - lastSecond >= 1000) {
    lastSecond = millis();
    if (timerRemain > 0) {
      timerRemain--;
      mqtt.publish(TOPIC_TIMER, String(timerRemain).c_str(), true);
    }
    if (timerRemain == 0) {
      setFan(false);
      timerActive = false;
      mqtt.publish(TOPIC_TIMER, "0", true);
      Serial.println("\n타이머 종료 - 팬 OFF\n");
    }
  }

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

  if (tempModeActive) {
    if (!fanOn && temp >= triggerTemp) {
      setFan(true);
      Serial.println("온도 제어: 팬 ON (현재 " + String(temp, 1) + "°C >= 가동 " + String(triggerTemp, 1) + "°C)\n");
    } else if (fanOn && temp <= targetTemp) {
      setFan(false);
      Serial.println("온도 제어: 팬 OFF (현재 " + String(temp, 1) + "°C <= 목표 " + String(targetTemp, 1) + "°C)\n");
    }
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("수신: [" + String(topic) + "] " + msg + "\n");

  if (String(topic) == TOPIC_TEMP_CTL) {
    if (msg == "0") {
      tempModeActive = false;
      mqtt.publish(TOPIC_TEMP_MODE, "0", true);
      Serial.println("온도 제어 모드 종료\n");
    } else {
      int sep = msg.indexOf(':');
      if (sep > 0) {
        triggerTemp    = msg.substring(0, sep).toFloat();
        targetTemp     = msg.substring(sep + 1).toFloat();
        tempModeActive = true;
        mqtt.publish(TOPIC_TEMP_MODE, msg.c_str(), true);
        Serial.println("온도 제어 시작: 가동 " + String(triggerTemp, 1) + "°C / 목표 " + String(targetTemp, 1) + "°C\n");
      }
    }
    return;
  }

  if (msg == "1") {
    timerActive    = false;
    tempModeActive = false;
    mqtt.publish(TOPIC_TEMP_MODE, "0", true);
    setFan(true);
    mqtt.publish(TOPIC_TIMER, "0", true);
  } else if (msg == "0") {
    timerActive    = false;
    tempModeActive = false;
    mqtt.publish(TOPIC_TEMP_MODE, "0", true);
    setFan(false);
    mqtt.publish(TOPIC_TIMER, "0", true);
  } else if (msg.startsWith("T")) {
    timerRemain = msg.substring(1).toInt();
    if (timerRemain > 0) {
      tempModeActive = false;
      mqtt.publish(TOPIC_TEMP_MODE, "0", true);
      timerActive = true;
      lastSecond  = millis();
      setFan(true);
      mqtt.publish(TOPIC_TIMER, String(timerRemain).c_str(), true);
    }
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
      mqtt.subscribe(TOPIC_TEMP_CTL);
      publishSensor();
    } else {
      Serial.println("MQTT 실패: " + String(mqtt.state()) + "\n");
      delay(3000);
    }
  }
}
