
#define BAT_PIN 1
#define CHARGE_PIN 2
#define R1_BAT 80000.0
#define R2_BAT 190000.0
#define R1_CHG 100000.0
#define R2_CHG 20000.0
#define ADC_RES 4095.0
#define ADC_REF 2.971
#define SAMPLES 16
#define CHARGE_THRESHOLD 2.28

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_PIN, ADC_11db);
  analogSetPinAttenuation(CHARGE_PIN, ADC_11db);
}

void loop() {
  // Read battery voltage
  int rawBat = 0;
  for (int i = 0; i < SAMPLES; i++) {
    rawBat += analogRead(BAT_PIN);
    delay(5);
  }
  rawBat /= SAMPLES;

  // Read charge pin
  int rawCharge = analogRead(CHARGE_PIN);

  float vAdc = (rawBat / ADC_RES) * ADC_REF;
  float vBat = vAdc * ((R1_BAT + R2_BAT) / R2_BAT);
  float vCharge = (rawCharge / ADC_RES) * ADC_REF;

  bool charging = vCharge > CHARGE_THRESHOLD;

  int percent = constrain((vBat - 3.0) / (4.2 - 3.0) * 100, 0, 100);

  Serial.printf("Battery: %.2fV  |  %d%%  |  Charging: %s  |  ChargePin: %.2fV\n",
                vBat, percent, charging ? "YES" : "NO", vCharge);

  delay(1000);
}
