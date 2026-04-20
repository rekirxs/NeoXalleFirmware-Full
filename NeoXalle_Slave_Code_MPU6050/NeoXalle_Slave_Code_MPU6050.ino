#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define ADXL375_ADDR 0x53
#define REG_POWER_CTL 0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0 0x32

#define SDA_PIN 6 
#define SCL_PIN 7  //blue pod

/* #define SDA_PIN 7
#define SCL_PIN 6 */ //blackpod

#define LED_PIN 2
#define LED_COUNT 24

#define HIT_THRESHOLD 30
#define DEBOUNCE_MS 500
#define LED_TIMEOUT 2000



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
  bool hit;
  float gs;
  int reactionMs;

} DataPacket;

DataPacket data;

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

  WiFi.mode(WIFI_STA);
  esp_now_init(); 

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  randomSeed(analogRead(0));

  Serial.println("Ready");
}

void loop() {

  static unsigned long nextLedTime = 0;
  if (!ledActive && millis() > nextLedTime) {
    ledOn();
    nextLedTime = millis() + random(3000,8000);
  }

  if (ledActive && millis() - ledOnTime > LED_TIMEOUT){
    Serial.println("Missed! LED off");
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

  int16_t dX = abs(rx - prevX);
  int16_t dY = abs(ry - prevY);
  int16_t dZ = abs(rz - prevZ); 

  int16_t delta = max({dX, dY, dZ});

  prevX = rx; prevY = ry; prevZ = rz;

  if (delta > HIT_THRESHOLD && millis() - lastHit > DEBOUNCE_MS) {
    lastHit = millis();

    if (ledActive) {
      int reactionMs = millis() - ledOnTime;
      float gs = delta * 0.049f;

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
