#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define MPU6050_ADDR 0x68
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT 0x3B

#define SDA_PIN 6
#define SCL_PIN 7
#define LED_PIN 2
#define LED_COUNT 24

#define HIT_THRESHOLD 1000
#define DEBOUNCE_MS 800
#define LED_TIMEOUT 2000


/* MASTER
Connected to ESP32-S3 on COM15:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
MAC:                e0:72:a1:d6:44:d8
*/


uint8_t masterAddress [] = { 0xe0, 0x72, 0xa1, 0xd6, 0x44, 0xd8};
typedef struct {
  bool turnOn;
} CommandPacket;

typedef struct {
  bool hit;
  float gs;
  int reactionMs;

} DataPacket;

DataPacket data;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastHit = 0;
unsigned long ledOnTime = 0;
bool ledActive = false;

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

void readAccel(int16_t &rx, int16_t &ry, int16_t &rz) {
   Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_ACCEL_XOUT);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR,6);

  rx = (Wire.read() << 8)| (Wire.read());
  ry = (Wire.read() << 8)| (Wire.read());
  rz = (Wire.read() << 8)| (Wire.read());

}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len){
  CommandPacket cmd;
  memcpy(&cmd, incomingData, sizeof(cmd));
  if(cmd.turnOn && !ledActive){
    ledOn();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(REG_PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission();

  strip.begin();
  strip.setBrightness(100);
  strip.show();

  WiFi.mode(WIFI_STA);
  esp_now_init(); 
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  Serial.println("Ready");
}

void loop() {
  if (ledActive && millis() - ledOnTime > LED_TIMEOUT){
    Serial.println("Missed! LED off");
    data.hit = false;
    data.gs = 0;
    data.reactionMs = -1;
    esp_now_send(masterAddress, (uint8_t *)&data, sizeof(data));
    ledOff();
  }
  int16_t rx, ry, rz;
  readAccel(rx, ry, rz);
 
  static int16_t prevX = 0, prevY = 0, prevZ = 0;

  int16_t dX = abs(rx - prevX);
  int16_t dY = abs(ry - prevY);
  int16_t dZ = abs(rz - prevZ); 

  int16_t delta = max({dX, dY, dZ});

  prevX = rx; prevY = ry; prevZ = rz;

  if (delta > HIT_THRESHOLD && millis() - lastHit > DEBOUNCE_MS) {
    lastHit = millis();

    if (ledActive) {
      int reactionMs = millis() - ledOnTime;
      float gs = delta / 16384.0f;

      Serial.print("HIT! Reaction time was: ");
      Serial.print(reactionMs);
      Serial.println("ms force: ");
      Serial.print(gs,1);
      Serial.println("G");

      data.hit = true;
      data.gs = gs;
      data.reactionMs = reactionMs;
      esp_now_send(masterAddress, (uint8_t *)&data, sizeof(data));

      ledOff();
    } else {
      Serial.println("Hit detected but led was anyways off, ignoredddd");
    }
   
  }
 delay(5);
}
