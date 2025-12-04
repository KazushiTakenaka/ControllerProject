
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPNowManager.h" // ESPNowManager.hをインクルードします
#include "DataStructures.h" // 構造体定義をインクルードします

/* 定数宣言 */
// ピン番号
const int SW_PINS[] = {15, 16, 17, 18, 14, 25, 26, 27};
const int SLD_SW_PINS[] = {32, 2, 33, 4, 22, 5, 23, 19};
const int BLE_LED = 21, WHITE_LED = 13;
const int ANALOG_READ1 = 34, ANALOG_READ2 = 36, BATTERY = 35;

// LEDC
const int BLUE_LED_CHANNEL = 0, WHITE_LED_CHANNEL = 1;
const int LEDC_FREQ = 5000, LEDC_RESOLUTION = 8;
const int RECEIVED_LED_BRIGHTNESS = 50;
const int BREATHING_EFFECT_PERIOD = 2000;

// バッテリー
const int LOW_BATTERY_THRESHOLD = 330; // 3.3V
const int BATTERY_WARNING_LED_BRIGHTNESS = 100;
const int R1 = 10000, R2 = 20000;

// タイマー
const int INTERVAL = 20; // 実行間隔（ミリ秒）

/* グローバル変数 */
uint8_t sender_mac[6];
int receivedDataLength = 0;
ReceivedDataPacket receivedPacket;
SaneDataPacket sendData;
ESPNowManager espNowManager;
unsigned long previousMillis = 0;

/* 関数宣言 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);
void initializePins();
void initializeLEDPins();
void handleBattery();
void handleLed();
void readSensorsAndSend();
int getVoltage();

void setup() {
  Serial.begin(115200);
  Serial.println("受信側 setup");

  Serial.print("自身のMACアドレス: ");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  espNowManager.init();
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  initializePins();
  initializeLEDPins();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= INTERVAL) {
    previousMillis = currentMillis;

    handleBattery();
    handleLed();
    if (espNowManager.isPaired) {
      readSensorsAndSend();
    } else {
      Serial.println("ペアリング待機中です...");
    }
  }
}

/* ESP-NOWコールバック */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // #if 0
  //   Serial.print("送信ステータス:\t");
  //   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
  // #endif
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  memcpy(sender_mac, mac_addr, 6);
  memcpy(&receivedPacket, incomingData, sizeof(receivedPacket));
  receivedDataLength = len;

  if (!espNowManager.isPaired) {
    espNowManager.pairDevice(sender_mac);
  }
}

/* 初期化関数 */
void initializePins() {
  for (int pin : SW_PINS) {
    pinMode(pin, INPUT_PULLUP);
  }
  for (int pin : SLD_SW_PINS) {
    pinMode(pin, INPUT_PULLUP);
  }
  pinMode(ANALOG_READ1, INPUT);
  pinMode(ANALOG_READ2, INPUT);
  
  analogSetPinAttenuation(ANALOG_READ1, ADC_11db);
  analogSetPinAttenuation(ANALOG_READ2, ADC_11db);
  analogReadResolution(12);
}

void initializeLEDPins() {
  ledcSetup(BLUE_LED_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(BLE_LED, BLUE_LED_CHANNEL);
  ledcSetup(WHITE_LED_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
  ledcAttachPin(WHITE_LED, WHITE_LED_CHANNEL);
}

/* loop内処理関数 */
void handleBattery() {
  int battery_value = getVoltage();
  Serial.print("電源電圧: ");
  Serial.print(battery_value);
  Serial.println(" mV");

  if (battery_value < LOW_BATTERY_THRESHOLD) {
    ledcWrite(WHITE_LED_CHANNEL, BATTERY_WARNING_LED_BRIGHTNESS);
  } else {
    ledcWrite(WHITE_LED_CHANNEL, 0);
  }
}

void handleLed() {
  if (receivedDataLength > 0) {
    ledcWrite(BLUE_LED_CHANNEL, RECEIVED_LED_BRIGHTNESS);
    receivedDataLength = 0; // 受信データをクリア
  } else if (!espNowManager.isPaired) {
    // ペアリングされていない場合の処理 (ブリージングエフェクト)
    float rad = (millis() % BREATHING_EFFECT_PERIOD) / (float)BREATHING_EFFECT_PERIOD * 2.0 * PI;
    int brightness = (int)((sin(rad - PI / 2.0) + 1.0) / 2.0 * 255);
    ledcWrite(BLUE_LED_CHANNEL, brightness);
  } else {
    // ペアリング済みでデータ受信がない場合は消灯
    ledcWrite(BLUE_LED_CHANNEL, 0);
  }
}

void readSensorsAndSend() {
  sendData.slideVal1 = map(analogRead(ANALOG_READ1), 0, 4095, 0, 255);
  sendData.slideVal2 = map(analogRead(ANALOG_READ2), 0, 4095, 0, 255);

  sendData.sld_sw1_1 = digitalRead(SLD_SW_PINS[0]);
  sendData.sld_sw1_2 = digitalRead(SLD_SW_PINS[1]);
  sendData.sld_sw2_1 = digitalRead(SLD_SW_PINS[2]);
  sendData.sld_sw2_2 = digitalRead(SLD_SW_PINS[3]);
  sendData.sld_sw3_1 = digitalRead(SLD_SW_PINS[4]);
  sendData.sld_sw3_2 = digitalRead(SLD_SW_PINS[5]);
  sendData.sld_sw4_1 = digitalRead(SLD_SW_PINS[6]);
  sendData.sld_sw4_2 = digitalRead(SLD_SW_PINS[7]);

  sendData.sw1 = digitalRead(SW_PINS[0]);
  sendData.sw2 = digitalRead(SW_PINS[1]);
  sendData.sw3 = digitalRead(SW_PINS[2]);
  sendData.sw4 = digitalRead(SW_PINS[3]);
  sendData.sw5 = digitalRead(SW_PINS[4]);
  sendData.sw6 = digitalRead(SW_PINS[5]);
  sendData.sw7 = digitalRead(SW_PINS[6]);
  sendData.sw8 = digitalRead(SW_PINS[7]);

  esp_err_t result = esp_now_send(sender_mac, (uint8_t *)&sendData, sizeof(sendData));
  if (result != ESP_OK) {
    Serial.printf("データ送信失敗: %d\n", result);
  }
}

/* ユーティリティ関数 */
int getVoltage() {
  int voltage_value = analogRead(BATTERY);
  double voltage = (voltage_value * 3.3 / 4095.0) * (double)(R1 + R2) / R2;
  return (int)(voltage * 100);
}
