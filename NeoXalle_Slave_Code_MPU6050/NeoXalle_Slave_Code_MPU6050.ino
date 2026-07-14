#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define POD_ID 4

#define VIBRO_PIN 10

#define MPU6050_ADDR 0x68
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT 0x3B

/*#define SDA_PIN 6
#define SCL_PIN 7
*/ // blue pod

#define SDA_PIN 7
#define SCL_PIN 6

#define LED_PIN 2
#define LED_COUNT 24

#define HIT_THRESHOLD 9000
#define DEBOUNCE_MS 500
#define LED_TIMEOUT 2000

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CMD_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define NOTIFY_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9" 

BLEServer* bleServer = nullptr;
BLECharacteristic* notifyChar = nullptr;
volatile bool deviceConnected = false;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastHit = 0;
unsigned long ledOnTime = 0;
bool ledActive = false;

void ledOn() {
  for (int i = 0; i < LED_COUNT; i++)
    strip.setPixelColor(i, strip.Color(0, 150, 255));
  strip.show();                  
  ledOnTime = millis();          
  ledActive = true;
  Serial.println("LED ON - waiting for hit...");
}

void ledOff() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0)); 
  }
  strip.show();
  ledActive = false;
}

void readAccel(int16_t &rx, int16_t &ry, int16_t &rz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR,6);

  rx = (Wire.read() << 8)| (Wire.read());
  ry = (Wire.read() << 8)| (Wire.read());
  rz = (Wire.read() << 8)| (Wire.read());

}

void sendResult(const char*  msg) {
  if (deviceConnected && notifyChar) {
    notifyChar->setValue(msg);
    notifyChar->notify();
  }
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    deviceConnected = true;
    Serial.println("App connected to this pod ");
  }
  void onDisconnect(BLEServer* s) override {
    deviceConnected = false;
    Serial.println("App disconected, re-advertising");
    delay(200);
    bleServer->startAdvertising();
  }
};

class CmdCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String val = String(c->getValue().c_str());
    val.trim();
    Serial.print("BLE cmd: ");
    Serial.println(val);

    if (val == "ON" && !ledActive) {
      ledOn();
    } else if (val == "OFF") {
      ledOff();
    } else if (val == "MOTOR_ON"){
      digitalWrite(VIBRO_PIN,HIGH);
    } else if (val == "MOTOR_OFF") {
      digitalWrite(VIBRO_PIN, LOW);
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(VIBRO_PIN, OUTPUT);
  digitalWrite(VIBRO_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission();

  strip.begin();
  strip.setBrightness(100);
  strip.show();

  String deviceName = "NeoXalle-Pod-" + String(POD_ID);
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
  int16_t rx, ry, rz;
  readAccel(rx, ry, rz);
 
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
      float gs = peakDelta / 16384.0f;

      Serial.print("HIT! delta: "); Serial.print(peakDelta);
      Serial.print(" force: "); Serial.print(gs, 1); Serial.println("G");

      if (ledActive) {
        int reactionMs = millis() - ledOnTime;
        char result[32];
        snprintf(result, sizeof(result), "HIT:%d:%1f", reactionMs, gs);
        sendResult(result);
        ledOff();
      }
    }
    peakPending = false;
    peakDelta   = 0;
  }

  delay(5);
}
