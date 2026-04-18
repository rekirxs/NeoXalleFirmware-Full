/*#include <WiFi.h>
#include <esp_now.h>*/
#include <Wire.h>

#define ADXL375_ADDR 0x53
#define REG_POWER_CTL 0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0 0x32

#define SDA_PIN 7
#define SCL_PIN 6

#define HIT_THRESHOLD 1.5f

void writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void readAccel (float &gx, float &gy, float &gz){
  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(REG_DATAX0);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL375_ADDR,6);

  int16_t rx = (Wire.read() | (Wire.read() << 8));
   int16_t ry = (Wire.read() | (Wire.read() << 8));
    int16_t rz = (Wire.read() | (Wire.read() << 8));

  gx = rx * 0.049f;
  gy = ry * 0.049f;
  gz = rz * 0.049f;
}

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

/*uint8_t masterAddress [] = { 0xe0, 0x72, 0xa1, 0xd6, 0x44, 0xd8};

typedef struct {
  float temp;
  int value;

} DataPacket;

DataPacket data;

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent OK" : "Send Failed"); 
}*/



void setup() {
  
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  writeReg(REG_DATA_FORMAT, 0x0B);
  writeReg(REG_POWER_CTL, 0x08);

  Serial.println("Ready");
 
/*
  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, masterAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
*/
  /*Serial.print("MAC Adress is: ");
  Serial.println(WiFi.macAddress());*/
}

void loop() {
  float gx, gy, gz;
  readAccel(gx, gy, gz);\

  float zImpact = abs(gz) - 1.0f;

  if (gz > HIT_THRESHOLD) {
    Serial.print("HIT! Z: ");
    Serial.print(gz, 1);
    Serial.println("G");
    delay(300);
  }
  /*data.temp = 2;
  data.value = 3;

  esp_now_send(masterAddress, (uint8_t *)&data, sizeof(data));
  delay(1000);*/
 delay(10);
}
