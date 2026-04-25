#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== PIN =====
#define BTN_MASUK 3
#define BTN_KELUAR 2

#define TMP36_PIN A2
#define MQ2_PIN A1
#define POT_PIN A0

#define AC_LED 5
#define VENT_LED 6

// ===== OBJECT =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== VAR =====
float temperature;
int humidity;
int mq2_value;
int people_count = 0;

String lastStatus = "OFF";

// debounce
unsigned long lastMasuk = 0;
unsigned long lastKeluar = 0;

// timing non-blocking
unsigned long lastUpdate = 0;

// ===== SETUP =====
void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  pinMode(BTN_MASUK, INPUT_PULLUP);
  pinMode(BTN_KELUAR, INPUT_PULLUP);

  pinMode(AC_LED, OUTPUT);
  pinMode(VENT_LED, OUTPUT);

  lcd.print("Smart AC Ready");
  delay(1500);
  lcd.clear();
}

// ===== READ BUTTON =====
void readButtons() {
  if (!digitalRead(BTN_MASUK) && millis() - lastMasuk > 200) {
    if (people_count < 10) people_count++;
    lastMasuk = millis();
  }

  if (!digitalRead(BTN_KELUAR) && millis() - lastKeluar > 200) {
    if (people_count > 0) people_count--;
    lastKeluar = millis();
  }
}

// ===== READ SENSOR =====
void readSensors() {
  int adcTemp = analogRead(TMP36_PIN);
  float voltage = adcTemp * (5.0 / 1023.0);
  temperature = (voltage - 0.5) * 100;

  int potValue = analogRead(POT_PIN);
  humidity = map(potValue, 0, 1023, 0, 100);

  mq2_value = analogRead(MQ2_PIN);
}

// ===== SMART LOGIC =====
String getACStatus() {

  String gasStatus = getGasStatus();

  // PRIORITY 1: gas
  if (gasStatus == "BAD") return "VENT";

  // PRIORITY 2: tidak ada orang
  if (people_count == 0) return "OFF";

  // ===== COMFORT SCORE =====
  int comfort = temperature + (humidity / 10) + (people_count * 2);

  // ===== HYSTERESIS =====
  if (lastStatus == "HIGH" && comfort > 30) return "HIGH";
  if (lastStatus == "MED" && comfort > 25 && comfort <= 35) return "MED";

  // ===== DECISION =====
  if (comfort > 35) return "HIGH";
  if (comfort > 28) return "MED";
  if (comfort > 22) return "LOW";

  return "OFF";
}

// ===== CONTROL OUTPUT =====
void controlOutput(String status) {

  // reset
  analogWrite(AC_LED, 0);
  digitalWrite(VENT_LED, LOW);

  if (status == "LOW") {
    analogWrite(AC_LED, 80);
  }
  else if (status == "MED") {
    analogWrite(AC_LED, 150);
  }
  else if (status == "HIGH") {
    analogWrite(AC_LED, 255);
  }
  else if (status == "VENT") {
    digitalWrite(VENT_LED, HIGH);
  }
}

// ===== GAS STATUS =====
String getGasStatus() {
  if (mq2_value > 280) return "BAD";
  else if (mq2_value > 150) return "WARN";
  else return "OK";
}

// ===== LCD =====
void updateLCD(String acStatus) {
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C ");

  lcd.print("H:");
  lcd.print(humidity);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(people_count);
  lcd.print(" ");

  lcd.print(acStatus);
  lcd.print(" ");

  lcd.print(getGasStatus());
}

// ===== LOOP =====
void loop() {
  readButtons();
  readSensors();

  if (millis() - lastUpdate > 500) {

    String acStatus = getACStatus();
    controlOutput(acStatus);
    updateLCD(acStatus);

    lastStatus = acStatus;

    // debug
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" | Hum: ");
    Serial.print(humidity);
    Serial.print(" | Gas: ");
    Serial.print(mq2_value);
    Serial.print(" | People: ");
    Serial.print(people_count);
    Serial.print(" | AC: ");
    Serial.println(acStatus);

    lastUpdate = millis();
  }
}