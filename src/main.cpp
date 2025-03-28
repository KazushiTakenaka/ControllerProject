#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
// クラスインクルード
#include "ESPNowManager.h" // ESPNowManager.hをインクルード

/* 変数宣言 */
uint8_t sender_mac[6]; // 送信側ESP32-AのMACアドレスを保存
int receivedDataLength = 0; // 受信データの長さを保存

// 各ピン番号定義
const int SW1 = 15;
const int SW2 = 16;
const int SW3 = 17;
const int SW4 = 18;
const int SW5 = 14;
const int SW6 = 25;
const int SW7 = 26;
const int SW8 = 27;
const int SLD_SW1_1 = 32;
const int SLD_SW1_2 = 2;
const int SLD_SW2_1 = 33;
const int SLD_SW2_2 = 4;
const int SLD_SW3_1 = 22;
const int SLD_SW3_2 = 5;
const int SLD_SW4_1 = 23;
const int SLD_SW4_2 = 19;
const int BLE_LED = 21;
const int LED_LED = 13;
const int BATTERY = 35;
// LEDCのチャンネル番号(0~15)
const int blueLedChannel = 0;
const int ledLedChannel = 1;
// スライドボリューム値読み取りピン
// const int ANALOG_READ1 = 12;
// const int ANALOG_READ2 = 13;
const int ANALOG_READ1 = 34;
const int ANALOG_READ2 = 36;//SVP

/*関数宣言*/
int getVoltage(); // 電源電圧を取得する
void initializePins(); // ピンモード設定する
void initializeLEDPins(); // LEDC初期化

ESPNowManager espNowManager; // ESPNowManagerのインスタンスを生成

// 送信するデータの構造体
struct SaneDataPacket {
  int slideVal1;  int slideVal2;
  int sld_sw1_1;  int sld_sw1_2;  int sld_sw2_1;  int sld_sw2_2;
  int sld_sw3_1;  int sld_sw3_2;  int sld_sw4_1;  int sld_sw4_2;
  int sw1;  int sw2;  int sw3;  int sw4;  int sw5;  int sw6;  int sw7;  int sw8;
  };

// 受信するデータの構造体(↓サンプル、他に受信したいデータがあれば追加、変更する)
// ※モジュール側の送信するデータの構造体と同じにする
struct ReceivedDataPacket {
  int val1;
  int val2;
  int val3;
  int val4;
  int val5;
};

// 送信完了時のコールバック
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  // 送信ステータスをシリアルモニタに表示
# if 0
  Serial.print("送信ステータス:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
# endif
}

ReceivedDataPacket receivedPacket;
// データ受信時のコールバック
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // 送信元のMACアドレスを保存
  memcpy(sender_mac, mac_addr, 6);
  
  // 受信データを構造体に変換して保存
  memcpy(&receivedPacket, incomingData, sizeof(receivedPacket));
  receivedDataLength = len; // 受信データの長さを設定

  // まだペアリングされていない場合のみペアリング
  if (!espNowManager.isPaired)
  {
    espNowManager.pairDevice(sender_mac);
  }
}


void setup()
{
  Serial.begin(115200); // シリアル通信を初期化
  Serial.println("受信側 setup");

  // 自身のMACアドレスを確認
  Serial.print("自身のMACアドレス: ");
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA); // WiFiをアクセスポイントとステーションモードに設定
  WiFi.disconnect(); // 既存の接続をクリア
  delay(100); // 少し待機
  espNowManager.init(); // ESPNowManagerを初期化

  // 送信・受信両方のコールバックを登録
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  initializePins(); // ピンモード設定関数を呼び出し
  initializeLEDPins(); // LEDC初期化関数を呼び出し
}

// 送信するデータの構造体をループ外で一度生成
SaneDataPacket sendData;

unsigned long previousMillis = 0; // 前回の時間を記録する変数
const int interval = 20; // 待機時間（ミリ秒）
bool first_receive = false; // 初回受信フラグ
int battery_value = 0; // BUTTRY電圧確認用
int slideVal1 = 0;
int slideVal2 = 0;
bool ledState = false; // LEDの状態を記録する変数
unsigned long previousBlinkMillis = 0; // LED点滅の前回の時間を記録する変数
const int blinkInterval = 500; // LED点滅の間隔（ミリ秒）

void loop() {
  unsigned long currentMillis = millis(); // 現在の時間を取得
  // 一定時間ごとに実行する処理
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // 前回の時間を更新
    battery_value = getVoltage(); // 電源電圧を取得
    // Serial.print("電源電圧: ");
    // Serial.print(battery_value); // 電源電圧を表示
    if (battery_value <= 160) {
      if (currentMillis - previousBlinkMillis >= blinkInterval) {
        previousBlinkMillis = currentMillis; // 前回の点滅時間を更新
        ledState = !ledState; // LEDの状態を反転
        ledcWrite(ledLedChannel, ledState ? 150 : 10); // LEDを点滅させる
      }
    } else {
      ledcWrite(ledLedChannel, 0); // LEDCでPWM出力
    }
    
    // 受信データがある場合、シリアルモニタに表示
    if (receivedDataLength > 0) {
      // 電圧が160以下の場合、LEDを点滅させる
    ledcWrite(blueLedChannel, 20); // LEDCでPWM出力
      receivedDataLength = 0; // 受信データをクリア
    } else {
      ledcWrite(blueLedChannel, 0); // LEDCでPWM出力
    }

    // ペアリングが完了している場合のみ送信
    slideVal1 = analogRead(ANALOG_READ1);
    slideVal2 = analogRead(ANALOG_READ2);
    if (espNowManager.isPaired) {
      // 送信するデータの値を設定
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
      
      // データを送信
      esp_err_t result = esp_now_send(sender_mac, (uint8_t *)&sendData, sizeof(sendData));
      if (result == ESP_OK) {
        // Serial.print("データ送信成功: ");
      } else {
        Serial.printf("データ送信失敗: %d\n", result);
      }
    }
    else {
      Serial.println("ペアリング待機中...");
      ledcWrite(blueLedChannel, 0); // LEDCでPWM出力
    }
  }
}

/*関数*/
// ピンモード設定
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
}

void initializeLEDPins() {
  const int freq = 5000;// 5kHzのPWM周波数
  const int resolution = 8;// 8bit分解能

  // LEDCの設定
  ledcSetup(blueLedChannel, freq, resolution);
  ledcAttachPin(BLE_LED, blueLedChannel);
  ledcSetup(ledLedChannel, freq, resolution);
  ledcAttachPin(LED_LED, ledLedChannel);
}

// 電源電圧を取得する関数
int getVoltage() {
  // ADCで値を読み取る
  int adc_value = analogRead(BATTERY);
  const int R1 = 10000;
  const int R2 = 10000;

  // 電圧を計算し、100倍して整数で返す
  int voltage = (adc_value * 3.3 / 4095.0 * (R1 + R2) / R2) * 100;

  return voltage;
}