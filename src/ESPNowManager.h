#ifndef ESPNOWMANAGER_H
#define ESPNOWMANAGER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

class ESPNowManager {
public:
    ESPNowManager();

    bool init();
    bool pairDevice(const uint8_t *mac_addr, int channel = 0, bool autoChannel = true);
    bool isPaired;

private:
    uint8_t sender_mac[6]; // 送信側ESP32-AのMACアドレスを保存
};

#endif // ESPNOWMANAGER_H