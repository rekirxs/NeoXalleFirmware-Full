#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define POD_ID 1

#define ADXL375_ADDR 0x53
#define REG_POWER_CTL 0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0 0x32

#define SDA_PIN 6 
#define SCL_PIN 7

//#define SDA_PIN 7
// #define SCL_PIN 6  //blackpod

#define LED_PIN 2
#define LED_COUNT 24

#define HIT_THRESHOLD 150
#define DEBOUNCE_MS 500
#define LED_TIMEOUT 2000

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CMD_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define NOTIFY_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLEServer* bleServer = nullptr;
BLECharacteristic* notifyChar = nullptr;
volatile bool deviceConnected = false;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastHit = 0;
unsigned long ledOnTime = 0;
bool ledActive = false;


void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void ledOn() {
  for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 150, 255)); 
  }

  strip.show();
  ledOnTime = millis();
  ledActive = true;
  Serial.println("Led on waiting for the hit");
}

void ledOff() {
  for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0)); 
  }
  strip.show();
  ledActive = false;
  
}

void sendResult(const char* msg) {
  if (deviceConnected && notifyChar) {
    notifyChar->setValue(msg);
    notifyChar->notify();
  }
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    deviceConnected = true;
    Serial.println("App connected to this pod");
  }
  void onDisconnect(BLEServer* s) override {
    deviceConnected = false; 
    Serial.println("App disconnected, re-advertising .)");
    delay(200);
    bleServer->startAdvertising();
  }
};

class CmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String val = String(c->getValue().c_str());
    val.trim();
    Serial.print("BLE cmd: "); Serial.print(val);

    if (val == "ON" && !ledActive) {
      ledOn();
    } else if(val == "OFF") {
      ledOff();
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  writeReg(REG_POWER_CTL, 0x00);
  writeReg(REG_DATA_FORMAT, 0x0F);
  writeReg(REG_POWER_CTL, 0x08);

  strip.begin();
  strip.setBrightness(100);
  strip.show();

  String deviceName = "Neoxalle-Pod-" + String(POD_ID);
  BLEDevice::init(deviceName.c_str());

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

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.print("Ready, advertising as ");
  Serial.println(deviceName);



  Serial.println("Ready");
}

void loop() {
  if (ledActive && millis() - ledOnTime > LED_TIMEOUT){
    Serial.println("Missed! LED off");
    sendResult("MISS");
    ledOff();
  }

  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(REG_DATAX0);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL375_ADDR,6);

  int16_t rx = (Wire.read() | (Wire.read() << 8));
  int16_t ry = (Wire.read() | (Wire.read() << 8));
  int16_t rz = (Wire.read() | (Wire.read() << 8));

  static int16_t prevX = 0, prevY = 0, prevZ = 0;
  static int16_t peakDelta = 0;
  static unsigned long peakTime = 0;
  static bool peakPending = false;

  int16_t dX = abs(rx - prevX);
  int16_t dY = abs(ry - prevY);
  int16_t dZ = abs(rz - prevZ); 

  int16_t delta = max({dX, dY, dZ});

  prevX = rx; prevY = ry; prevZ = rz;

  if (delta > HIT_THRESHOLD) {
    if (!peakPending) {
      peakPending = true;
      peakDelta   = delta;
      peakTime    = millis();
    } else if (delta > peakDelta) {
      peakDelta = delta;
    }
  }

 
  if (peakPending && millis() - peakTime > 150) {
    if (millis() - lastHit > DEBOUNCE_MS) {
      lastHit = millis();
      float gs = peakDelta * 0.049f;

      Serial.print("HIT! delta: "); Serial.print(peakDelta);
      Serial.print(" force: "); Serial.print(gs, 1); Serial.println("G");

      if (ledActive) {
        int reactionMs = millis() - ledOnTime;
        char result[32];
        snprintf(result, sizeof(result), "HIT:%d:#.1f", reactionMs, gs);
        ledOff();
      }
    }
    peakPending = false;
    peakDelta   = 0;
  }

  delay(5);
}
