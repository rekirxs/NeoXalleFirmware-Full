#include <WiFi.h>
#include <esp_now.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

uint8_t pod1[] = {0x20, 0x6E, 0xF1, 0x6B, 0xAD, 0xAC};
uint8_t pod2[] = {0xA8, 0x46, 0x74, 0x47, 0xE2, 0xB0};
uint8_t pod3[] = {0x20, 0x6E, 0xF1, 0x67, 0x2E, 0x84};
uint8_t pod4[] = {0x50, 0x78, 0x7d, 0x62, 0x49, 0xe8};

uint8_t* pods[] = {pod1, pod2, pod3, pod4};

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CMD_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define NOTIFY_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLEServer* bleServer = nullptr;
BLECharacteristic* notifyChar = nullptr;
bool deviceConnected = false;

typedef struct {
  bool turnOn; 
  bool turnOff;
} CommandPacket;
 
typedef struct {
  bool hit;
  float gs;
  int reactionMs;
} DataPacket;

DataPacket received;
CommandPacket cmd;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) {
    deviceConnected = true;
    Serial.println("App connected");
  }
  void onDisconnect(BLEServer* s){
    deviceConnected = false;
    Serial.println("App disconnected");
    bleServer->startAdvertising();
  }
};
class CmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c){
    String val = c->getValue().c_str();
    Serial.print("BLE cmd: "); Serial.println(val);

    if (val == "OFF:ALL"){
      cmd.turnOn = false;
      cmd.turnOff = true;
      for (int i = 0; i < 4; i++)
        esp_now_send(pods[i], (uint8_t *)&cmd, sizeof(cmd));
    } else if (val.startsWith("ON")){
      int podIndex = val.substring(3).toInt() - 1;
      if (podIndex >= 0 && podIndex < 4) {
        cmd.turnOn = true;
        cmd.turnOff = false;
        esp_now_send(pods[podIndex], (uint8_t *)&cmd, sizeof(cmd));
        Serial.print("Triggered pod" ); Serial.println(podIndex + 1);
      }
    }
  }
};
void onEspReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&received, incomingData, sizeof(received));

  // find which pod sent it
  int podNum = -1;
  for (int i = 0; i < 4; i++) {
    if (memcmp(info->src_addr, pods[i], 6) == 0) {
      podNum = i + 1;
      break;
    }
  }

  // debug
  Serial.print("hit: "); Serial.print(received.hit);
  Serial.print(" gs: "); Serial.print(received.gs);
  Serial.print(" reactionMs: "); Serial.println(received.reactionMs);

  char result[64];
  if (received.reactionMs == -1) {
    snprintf(result, sizeof(result), "MISS:%d", podNum);
  } else {
    snprintf(result, sizeof(result), "HIT:%d:%d:%.1f", podNum, received.reactionMs, received.gs);
  }

  Serial.println(result);

  if (deviceConnected) {
    notifyChar->setValue(result);
    notifyChar->notify();
  }
}



void setup() {
  Serial.begin(115200);
  delay(1000);
 
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onEspReceive);
  
  esp_now_peer_info_t peer = {};
  peer.channel = 0;
  peer.encrypt = false;
  for (int i = 0; i < 4; i++) {
    memcpy(peer.peer_addr, pods[i], 6);
    esp_now_add_peer(&peer);
  }


  BLEDevice::init("NeoXalle-Master");
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  BLEService* service = bleServer->createService(SERVICE_UUID);

  BLECharacteristic* cmdChar = service->createCharacteristic(
    CMD_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
  cmdChar->setCallbacks(new CmdCallbacks());

  notifyChar = service->createCharacteristic(
  NOTIFY_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  notifyChar->addDescriptor(new BLE2902());

  service->start();
  bleServer->getAdvertising()->start();

  Serial.println("Master ready - advertising as NeoXalle-Master");
  
}
void loop() {
  
  
}
