/*#include <Wire.h>

#define ADXL375_ADDR 0x53
#define REG_POWER_CTL 0x2D
#define REG_DATA_FORMAT 0x31
#define REG_THRESH_SHOCK 0x1D
#define REG_DUR 0x21
#define REG_TAP_AXES 0x2A
#define REG_INT_ENABLE 0x2E
#define REG_INT_MAP 0x2F
#define REG_INT_SOURCE 0x30   

#define SDA_PIN 6
#define SCL_PIN 7
#define INT_PIN 3

void writeReg(uint8_t reg, uint8_t val){
  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void IRAM_ATTR onInterrupt() {
  Serial.println("INT triggered!");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  
  writeReg(REG_POWER_CTL, 0x00);
  writeReg(REG_DATA_FORMAT, 0x0F);
  writeReg(REG_THRESH_SHOCK, 0x10);
  writeReg(REG_DUR, 0x20);
  writeReg(REG_TAP_AXES, 0x07);
  writeReg(REG_INT_ENABLE, 0x40);
  writeReg(REG_INT_MAP,0x00);
  writeReg(REG_POWER_CTL,0x08);

  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), onInterrupt, RISING);

  Serial.println("Ready, tap it godamnit");


}

void loop() {
  
  Wire.beginTransmission(ADXL375_ADDR);
  Wire.write(REG_INT_SOURCE);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL375_ADDR, 1);
  Wire.read();

  delay(100);

}*/
#include <Wire.h>

#define SDA_PIN 6
#define SCL_PIN 7

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  Serial.println("Scanning I2C...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Device found at 0x");
      Serial.println(addr, HEX);
    }
  }
  Serial.println("Done.");
}

void loop() {}
