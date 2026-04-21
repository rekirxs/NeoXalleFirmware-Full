#include <WiFi.h>
#include <esp_now.h>

uint8_t pod1[] = {0x20, 0x6E, 0xF1, 0x6B, 0xAD, 0xAC};
uint8_t pod2[] = {0xA8, 0x46, 0x74, 0x47, 0xE2, 0xB0};
uint8_t pod3[] = {0x20, 0x6E, 0xF1, 0x67, 0x2E, 0x84};

uint8_t* pods[] = {pod1, pod2, pod3};

typedef struct {
  bool turnOn; 
} CommandPacket;
 
typedef struct {
  bool hit;
  float gs;
  int reactionMs;
} DataPacket;

DataPacket received;
CommandPacket cmd;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&received, incomingData, sizeof(received));

  Serial.print("pod [");
  for (int i = 0; i < 6; i++) {
    Serial.print(info->src_addr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.print("] ");

  if(received.reactionMs == -1) {
    Serial.println("Missed");
    
  } else {
    Serial.print("Hit, reaction: ");
    Serial.print(received.reactionMs);
    Serial.print("ms force: ");
    Serial.print(received.gs,1);
    Serial.println("G");
  }
}

void sendTurnOn(uint8_t *adress){
  cmd.turnOn= true;
  esp_now_send(adress, (uint8_t *)&cmd, sizeof(cmd));
}

void setup() {
  Serial.begin(115200);
  delay(1000);
 
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onReceive);
  
  esp_now_peer_info_t peer = {};
  peer.channel = 0;
  peer.encrypt = false;

  memcpy(peer.peer_addr, pod1, 6);
  esp_now_add_peer(&peer);  
  
  memcpy(peer.peer_addr, pod2, 6);
  esp_now_add_peer(&peer);  

  memcpy(peer.peer_addr, pod3, 6);
  esp_now_add_peer(&peer);    
  
  randomSeed(analogRead(0));
  Serial.println("Master Ready");
}

void loop() {
  
  static unsigned long nextTrigger = 0;

  if (millis() > nextTrigger) {
    int podIndex = random(3);
    Serial.print("Triggering pod ");
    Serial.println(podIndex + 1);

    sendTurnOn(pods[podIndex]);
    nextTrigger = millis() + random(3000, 8000);
  }
}
