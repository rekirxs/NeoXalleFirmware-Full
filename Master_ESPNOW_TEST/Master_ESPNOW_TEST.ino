#include <WiFi.h>
#include <esp_now.h>

typedef struct {
  float temp;
  int value;
} DataPacket;

DataPacket receivedData;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&receivedData, data, sizeof(receivedData));
  Serial.print("Temp : ");
  Serial.println(receivedData.temp);
  Serial.print("Value: ");
  Serial.println(receivedData.value);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_recv_cb(onReceive);
}

void loop() {
  

}
