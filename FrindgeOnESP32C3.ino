#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Wire.h>

// Pin Definitions (переназначены под ESP32-C3)
#define SPEAKER_PIN 0
#define TEMPERATURE_SENSOR_PIN 1
#define BUTTON_PIN 6
#define RELAY_PIN 21
#define Encoder_A_Pin_Interrupt 5
#define Encoder_B_Pin 7
// Screen pins SDA=8, SCL=9

// Initialize OneWire and DallasTemperature objects
OneWire oneWire(TEMPERATURE_SENSOR_PIN);
DallasTemperature temperatureSensor(&oneWire);

// OLED Display Configuration
#define SCREEN_I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET_PIN -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

// Global Variables
float currentTemperature = 0;
const int activationTemperature = 4;
float deactivationTemperature = -4;
float deactivationTemperaturePREV;
float minimumMinusTemperature = -9;

int currentStateEncoder_A_Pin_Interrupt;
int previousStateEncoder_A_Pin_Interrupt;

bool screenTriggerEncoder = false;
bool screenTriggerButton = false;
bool RELAY_PIN_FLAG = false;

String temperatureString;

unsigned long previousMillis = 0;
unsigned long currentMillis;

// EEPROM Variables
const int EEPROM_ADDRESS = 0;
// float tempVar = -1;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(Encoder_A_Pin_Interrupt, INPUT);
  pinMode(Encoder_B_Pin, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

  temperatureSensor.begin();

  Wire.begin(8, 9); // SDA, SCL
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDRESS);
  display.dim(1);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.setTextWrap(false);
  display.dim(0);

  attachInterrupt(digitalPinToInterrupt(Encoder_A_Pin_Interrupt), encoder, CHANGE);
  previousStateEncoder_A_Pin_Interrupt = digitalRead(Encoder_A_Pin_Interrupt);

  EEPROM.begin(64); // обязательно для ESP32
  // EEPROM.put(EEPROM_ADDRESS, tempVar);
  EEPROM.get(EEPROM_ADDRESS, deactivationTemperature);

  powerOn();
}

void loop() {
  currentMillis = millis();
  updateTemperatureEveryTenSeconds();
  handleTemperatureSettings();
  controlRelay();
}

void updateTemperatureEveryTenSeconds() {
  if (currentMillis - previousMillis > 10000) {
    readTemperature();
    previousMillis = currentMillis;
  }
}

void handleTemperatureSettings() {
  if (screenTriggerEncoder) {
    displayTemperatureBorders();
    if (currentMillis - previousMillis >= 3000) {
      screenTriggerEncoder = false;
    }
  } else {
    if (digitalRead(BUTTON_PIN) == LOW) {
      screenTriggerButton = true;
      previousMillis = currentMillis;
    }
    if ((currentMillis - previousMillis < 3000) && screenTriggerButton) {
      displayTemperatureBorders();
    } else {
      screenTriggerButton = false;
      displayCurrentTemperature();
    }
  }

  if (deactivationTemperaturePREV != deactivationTemperature) {
    deactivationTemperaturePREV = deactivationTemperature;
    screenTriggerEncoder = true;
    previousMillis = currentMillis;
  }
}

void controlRelay() {
  if (currentTemperature >= activationTemperature || currentTemperature == -127) {
    digitalWrite(RELAY_PIN, HIGH);
    RELAY_PIN_FLAG = true;
  }
  if (currentTemperature <= deactivationTemperature && currentTemperature != -127) {
    digitalWrite(RELAY_PIN, LOW);
    RELAY_PIN_FLAG = false;
  }
}

void displayCurrentTemperature() {
  display.clearDisplay();
  temperatureString = String(currentTemperature);
  temperatureString.remove(temperatureString.length() - 1); // фикс строки

  display.setCursor((SCREEN_WIDTH - (temperatureString.length() - 1) * 24) / 2, (SCREEN_HEIGHT - 28) / 2);
  display.setTextSize(4, 4);
  if (currentTemperature == -127) {
    display.setTextSize(2, 2);
    display.setCursor(10, 20);
    display.print("SensorERR");
  } else {
    display.print(temperatureString);
  }
  if (RELAY_PIN_FLAG) {
    display.setTextSize(1);
    display.setCursor(5, 3);
    display.print("Active");
  } else {
    display.setTextSize(1);
    display.setCursor(95, 3);
    display.print("Sleep");
  }
  display.display();
}

void readTemperature() {
  temperatureSensor.requestTemperatures();
  currentTemperature = temperatureSensor.getTempCByIndex(0);
}

void displayTemperatureBorders() {
  for (int i = 15; i <= 64; i++) {
    display.drawLine(0, i, 128, i, 0x0000);
  }

  display.setTextSize(2, 2);
  display.setCursor(7, 20);
  display.println("High");

  display.setCursor(7, 45);
  display.println("Low");

  display.setCursor(110, 20);
  display.println(activationTemperature);

  if (deactivationTemperature == 0) {
    display.setCursor(110, 45);
    display.println(deactivationTemperature, 0);
  } else {
    display.setCursor(74, 45);
    display.println(deactivationTemperature, 1);
  }

  display.display();
}

void encoder() {
  currentStateEncoder_A_Pin_Interrupt = digitalRead(Encoder_A_Pin_Interrupt);

  if (currentStateEncoder_A_Pin_Interrupt != previousStateEncoder_A_Pin_Interrupt) {
    if (digitalRead(Encoder_B_Pin) != currentStateEncoder_A_Pin_Interrupt) {
      if (!(deactivationTemperature <= minimumMinusTemperature)) {
        deactivationTemperature = deactivationTemperature - 0.5;
        EEPROM.put(EEPROM_ADDRESS, deactivationTemperature);
        EEPROM.commit();
      }
    } else {
      if (!(deactivationTemperature >= 0)) {
        deactivationTemperature = deactivationTemperature + 0.5;
        EEPROM.put(EEPROM_ADDRESS, deactivationTemperature);
        EEPROM.commit();
      }
    }
    previousStateEncoder_A_Pin_Interrupt = currentStateEncoder_A_Pin_Interrupt;
  }
}

void powerOn() {
  for (int i = 1; i <= 3; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(80 * 1.3);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(50 * 1.3);
  }
  delay(80 * 1.3);
  for (int i = 1; i <= 3; i++) {
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(80 * 1.3);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(150 * 1.3);
  }
}
