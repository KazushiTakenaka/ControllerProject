#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "ESPNowManager.h" // ESPNowManager.hをインクルードします

/* 変数宣言 */
uint8_t sender_mac[6]; // 送信元のESP32-AのMACアドレスを保存します
int receivedDataLength = 0; // 受信データの長さを保存します

// 各ピン番号を定義します
const int SW1 = 15, SW2 = 16, SW3 = 17, SW4 = 18, SW5 = 14, SW6 = 25, SW7 = 26, SW8 = 27;
const int SLD_SW1_1 = 32, SLD_SW1_2 = 2, SLD_SW2_1 = 33, SLD_SW2_2 = 4;
const int SLD_SW3_1 = 22, SLD_SW3_2 = 5, SLD_SW4_1 = 23, SLD_SW4_2 = 19;
const int BLE_LED = 21, WHITE_LED = 13;
const int blueLedChannel = 0, whiteLedChannel = 1; // LEDCのチャンネル番号を定義します
const int ANALOG_READ1 = 34, ANALOG_READ2 = 36, BATTERY= 35; // アナログ入力ピンを定義します

/* 関数宣言 */
int getVoltage(); // 電源電圧を取得します
void initializePins(); // ピンモードを設定します
void initializeLEDPins(); // LEDCを初期化します

ESPNowManager espNowManager; // ESPNowManagerのインスタンスを生成します

// 送信するデータの構造体を定義します
struct SaneDataPacket {
  int slideVal1, slideVal2;
  int sld_sw1_1, sld_sw1_2, sld_sw2_1, sld_sw2_2;
  int sld_sw3_1, sld_sw3_2, sld_sw4_1, sld_sw4_2;
  int sw1, sw2, sw3, sw4, sw5, sw6, sw7, sw8;
};

// 受信するデータの構造体を定義します
struct ReceivedDataPacket {
  int val1, val2, val3, val4, val5;
};

// 送信完了時のコールバック関数です
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // 送信ステータスをシリアルモニタに表示します
# if 0
  Serial.print("送信ステータス:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
# endif
}

ReceivedDataPacket receivedPacket;
// データ受信時のコールバック関数です
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  // 送信元のMACアドレスを保存します
  memcpy(sender_mac, mac_addr, 6);
  
  // 受信データを構造体に変換して保存します
  memcpy(&receivedPacket, incomingData, sizeof(receivedPacket));
  receivedDataLength = len; // 受信データの長さを設定します

  // まだペアリングされていない場合のみペアリングします
  if (!espNowManager.isPaired) {
    espNowManager.pairDevice(sender_mac);
  }
}

void setup() {
  Serial.begin(115200); // シリアル通信を初期化します
  Serial.println("受信側 setup");

  // 自身のMACアドレスを確認します
  Serial.print("自身のMACアドレス: ");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA); // WiFiをアクセスポイントとステーションモードに設定します
  WiFi.disconnect(); // 既存の接続をクリアします
  delay(100); // 少し待機します
  espNowManager.init(); // ESPNowManagerを初期化します

  // 送信・受信両方のコールバックを登録します
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  initializePins(); // ピンモード設定関数を呼び出します
  initializeLEDPins(); // LEDC初期化関数を呼び出します
}

SaneDataPacket sendData; // 送信データを格納する構造体を生成します
unsigned long previousMillis = 0; // 前回の時間を記録します
const int interval = 20; // 実行間隔（ミリ秒）を設定します
bool first_receive = false; // 初回受信フラグです
int battery_value = 0; // バッテリー電圧を記録します
int slideVal1 = 0, slideVal2 = 0; // スライドボリュームの値を記録します
bool ledState = false; // LEDの状態を記録します
unsigned long previousBlinkMillis = 0; // LED点滅の前回時間を記録します
const int blinkInterval = 500; // LED点滅の間隔（ミリ秒）を設定します

void loop() {
  unsigned long currentMillis = millis(); // 現在の時間を取得します
  // 一定時間ごとに実行する処理です
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // 前回の時間を更新します
    battery_value = getVoltage(); // 電源電圧を取得します
    Serial.print("電源電圧: ");
    Serial.print(battery_value); // 電源電圧を表示します
    Serial.println(" mV");
    if (battery_value < 330) {
      ledcWrite(whiteLedChannel, 100); // 電源電圧が低い場合、白色LEDを最大点灯します
    } else {
      ledcWrite(whiteLedChannel, 0); // 電源電圧が正常な場合、白色LEDを消灯します
    }
    
    // 受信データがある場合、シリアルモニタに表示します
    if (receivedDataLength > 0) {
      ledcWrite(blueLedChannel, 50); // LEDCでPWM出力します
      receivedDataLength = 0; // 受信データをクリアします
    } else {
      // ペアリングされていない場合の処理です (ブリージングエフェクト)
      // 2000ms (2秒)周期で明るさを計算します
      float rad = (millis() % 2000) / 2000.0 * 2.0 * PI;
      // sinカーブを使い、0-255の範囲で滑らかな明るさの変化を生成します
      int brightness = (int)((sin(rad - PI / 2.0) + 1.0) / 2.0 * 255);
      ledcWrite(blueLedChannel, brightness);
    }

    slideVal1 = analogRead(ANALOG_READ1);
    slideVal2 = analogRead(ANALOG_READ2);
    // Serial.print(slideVal1);
    // Serial.print(",");
    // Serial.println(slideVal2);
    if (espNowManager.isPaired) {
      // データを送信します
      sendData.slideVal1 = map(slideVal1, 0, 4095, 0, 255);
      sendData.slideVal2 = map(slideVal2, 0, 4095, 0, 255);
      sendData.sld_sw1_1 = digitalRead(SLD_SW1_1);
      sendData.sld_sw1_2 = digitalRead(SLD_SW1_2);
      sendData.sld_sw2_1 = digitalRead(SLD_SW2_1);
      sendData.sld_sw2_2 = digitalRead(SLD_SW2_2);
      sendData.sld_sw3_1 = digitalRead(SLD_SW3_1);
      sendData.sld_sw3_2 = digitalRead(SLD_SW3_2);
      sendData.sld_sw4_1 = digitalRead(SLD_SW4_1);
      sendData.sld_sw4_2 = digitalRead(SLD_SW4_2);
      sendData.sw1 = digitalRead(SW1);
      sendData.sw2 = digitalRead(SW2);
      sendData.sw3 = digitalRead(SW3);
      sendData.sw4 = digitalRead(SW4);
      sendData.sw5 = digitalRead(SW5);
      sendData.sw6 = digitalRead(SW6);
      sendData.sw7 = digitalRead(SW7);
      sendData.sw8 = digitalRead(SW8);

      // データを送信します
      esp_err_t result = esp_now_send(sender_mac, (uint8_t *)&sendData, sizeof(sendData));
      if (result == ESP_OK) {
        // Serial.print("データ送信成功: ");
      } else {
        Serial.printf("データ送信失敗: %d\n", result);
      }
    } else {
      Serial.println("ペアリング待機中です...");
      // ペアリングされていない場合の処理です (ブリージングエフェクト)
      // 2000ms (2秒)周期で明るさを計算します
      float rad = (millis() % 2000) / 2000.0 * 2.0 * PI;
      // sinカーブを使い、0-255の範囲で滑らかな明るさの変化を生成します
      int brightness = (int)((sin(rad - PI / 2.0) + 1.0) / 2.0 * 255);
      ledcWrite(blueLedChannel, brightness);
    }
  }
}

/* 関数 */
// ピンモードを設定します
void initializePins() {
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  pinMode(SW4, INPUT_PULLUP);
  pinMode(SW5, INPUT_PULLUP);
  pinMode(SW6, INPUT_PULLUP);
  pinMode(SW7, INPUT_PULLUP);
  pinMode(SW8, INPUT_PULLUP);
  pinMode(SLD_SW1_1, INPUT_PULLUP);
  pinMode(SLD_SW1_2, INPUT_PULLUP);
  pinMode(SLD_SW2_1, INPUT_PULLUP);
  pinMode(SLD_SW2_2, INPUT_PULLUP);
  pinMode(SLD_SW3_1, INPUT_PULLUP);
  pinMode(SLD_SW3_2, INPUT_PULLUP);
  pinMode(SLD_SW4_1, INPUT_PULLUP);
  pinMode(SLD_SW4_2, INPUT_PULLUP);
  pinMode(ANALOG_READ1, INPUT);
  pinMode(ANALOG_READ2, INPUT);
  // アナログピンの設定をします
  analogSetPinAttenuation(ANALOG_READ1, ADC_11db); // ANALOG_READ1の減衰設定をします
  analogSetPinAttenuation(ANALOG_READ2, ADC_11db); // ANALOG_READ2の減衰設定をします
  analogReadResolution(12); // ADCの分解能を設定します
}

// LEDCを初期化します
void initializeLEDPins() {
  const int freq = 5000, resolution = 8; // PWMの設定です
  ledcSetup(blueLedChannel, freq, resolution);
  ledcAttachPin(BLE_LED, blueLedChannel);
  ledcSetup(whiteLedChannel, freq, resolution);
  ledcAttachPin(WHITE_LED, whiteLedChannel);
}

// 電源電圧を取得します
int getVoltage() {
  int voltage_value = analogRead(BATTERY);
  // 分圧抵抗の値 (実際の回路に合わせてください)
  const int R1 = 10000;
  const int R2 = 20000;

  // 電圧を計算します (ESP32のADCは3.3V基準、12bit分解能(0-4095))
  // 計算式: Vout = Vin * R2 / (R1 + R2) -> Vin = Vout * (R1 + R2) / R2
  // Vout = analogRead値 * 3.3 / 4095.0
  // 注意: ESP32のADC基準電圧は内部で変動することがあるため、より正確な測定にはキャリブレーションが必要です。
  double voltage = (voltage_value * 3.3 / 4095.0) * (double)(R1 + R2) / R2;

  return (int)(voltage * 100);
}