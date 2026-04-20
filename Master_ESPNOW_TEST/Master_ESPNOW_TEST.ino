#include <WiFi.h>
#include <esp_now.h>

typedef struct {
  bool hit;
  float gs;
  int reactionMs;
} DataPacket;

DataPacket received;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&received, incomingData, sizeof(received));

  if(received.hit) {
    Serial.print("HIT From pod [");
    for (int i=0; i<6; i++) {
      Serial.print(info->src_addr[i], HEX);
      if (i < 5) Serial.print(":");
    }
    Serial.print("] reaction: ");
    Serial.print(received.reactionMs);
    Serial.print("ms force: ");
    Serial.print(received.gs,1);
    Serial.println("G");
    
  }
}
  

void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_recv_cb(onReceive);
  Serial.println("Master Ready");
}

void loop() {
  

}
