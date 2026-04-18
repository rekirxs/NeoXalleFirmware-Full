#include <WiFi.h>
#include <esp_now.h>

/* SLAVE
 Chip type:          ESP32-C3 (QFN32) (revision v0.4)
Features:           Wi-Fi, BT 5 (LE), Single Core, 160MHz, Embedded Flash 4MB (XMC)
Crystal frequency:  40MHz
USB mode:           USB-Serial/JTAG
MAC:                20:6e:f1:6b:ad:ac*/ 

/* MASTER
Connected to ESP32-S3 on COM15:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
MAC:                e0:72:a1:d6:44:d8
*/

uint8_t masterAddress [] = { 0xe0, 0x72, 0xa1, 0xd6, 0x44, 0xd8};

typedef struct {
  float temp;
  int value;

} DataPacket;

DataPacket data;

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send Failed");
}



void setup() {
  
  Serial.begin(115200);
 

  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  /*Serial.print("MAC Adress is: ");
  Serial.println(WiFi.macAddress());*/
}

void loop() {
  data.temp = 2;
  data.value = 3;

  esp_now_send(masterAddress, (uint8_t *)&data, sizeof(data));
  delay(1000);

}
