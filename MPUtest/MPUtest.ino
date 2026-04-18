#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define DEVICE_NAME         "NeoXalle-Master"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("BLE CLient connected");
  }
  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("BLE Client disconnected - staying alive");
  }
};
void pushLog(const String& msg) {
  if(!deviceConnected) return;
  pCharacteristic->setValue(msg);
  pCharacteristic->notify();
  Serial.print("[TX] "); Serial.println(msg);

}
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String cmd = pCharacteristic->getValue();
    if (cmd.length()>0) {
      Serial.print("[CMD] "); Serial.println(cmd);
      pushLog("[LOG] CMD: " + cmd);
    }
  }
};



void bleWatchdogTask(void* param) {
  for (;;) {
    if (!deviceConnected){
      BLEAdvertising* adv = BLEDevice::getAdvertising();
      if (!pServer->getConnectedCount()) {
        adv->start();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println("[SYS] Starting NeoXalle BLE...");

  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
     CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ   |
    BLECharacteristic::PROPERTY_WRITE  |
    BLECharacteristic::PROPERTY_NOTIFY |
    BLECharacteristic::PROPERTY_INDICATE
  );
  pCharacteristic->setCallbacks(new CharacteristicCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue(String("NeoXalle ready"));

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising as: NeoXalle-Master");

  xTaskCreatePinnedToCore(bleWatchdogTask, "BLEWatchdog", 4096, NULL, 1, NULL, 0);
}

void loop(){
  if (deviceConnected) {
    static unsigned long lastNotify = 0;
    if (millis() - lastNotify > 2000) {
      lastNotify = millis();
      pushLog("[HB] uptime:" + String(millis() / 1000) + "s");
    }
  }
  delay(10);
}