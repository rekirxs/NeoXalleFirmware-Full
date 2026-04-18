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


#define BAT_PIN 4
#define CHARGE_PIN 5

#define SAMPLES 16
#define CHARGE_THRESHOLD 2.0


class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("BLE Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("BLE Client disconnected - restarting advertising");


    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->stop();
    delay(100);
    pAdvertising->start();
  }
};

void pushLog(const String& msg) {
  Serial.print("[TX] ");
  Serial.println(msg);

  if (!deviceConnected) return;

  pCharacteristic->setValue(msg);
  pCharacteristic->notify();
}

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String cmd = pCharacteristic->getValue();
    if (cmd.length() > 0) {
      Serial.print("[CMD] ");
      Serial.println(cmd);
      pushLog("[LOG] CMD: " + cmd);
    }
  }
};


void setup() {
  Serial.begin(115200);
  Serial.println("[SYS] Starting NeoXalle BLE + Battery...");

  analogReadResolution(12);
  analogSetPinAttenuation(BAT_PIN, ADC_11db);
  analogSetPinAttenuation(CHARGE_PIN, ADC_11db);


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
  pCharacteristic->setValue("NeoXalle ready");

  pService->start();


  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);

  BLEDevice::startAdvertising();

  Serial.println("[BLE] Advertising as: NeoXalle-Master");
}

void loop() {
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();

   
    int rawBat = 0;
    for (int i = 0; i < SAMPLES; i++) {
      rawBat += analogRead(BAT_PIN);
      delay(5);
    }
    rawBat /= SAMPLES;


    float vAdc = analogReadMilliVolts(BAT_PIN) / 1000.0;
    float vBat = vAdc * 1.48;  // tu factor calibrado

    int rawCharge = analogRead(CHARGE_PIN);
    float vCharge = analogReadMilliVolts(CHARGE_PIN) / 1000.0; 

    bool charging = vCharge > 0.4;

   // int percent = constrain((vBat - 3.0) / (4.2 - 3.0) * 100, 0, 100);

    int percent = constrain((vBat - 3.0) / (4.2 - 3.0) * 100, 0, 100);

    /* String msg = "raw:" + String(rawBat) +
                 " | vAdc:" + String(vAdc, 3) +
                 "V | vBat:" + String(vBat, 2) +
                 "V | " + String(percent) +
                 "% | CHG:" + (charging ? "YES" : "NO"); */

     String msg = "BAT:" + String(vBat, 2) +
                 " | V:" + String(percent) +
                 "% | CHG_PIN:" + String(vCharge, 3) +
                 "V | rawC:" + String(rawCharge) +
                 " | CHG:" + (charging ? "YES" : "NO");         

    pushLog(msg);
  }

  delay(10);
}