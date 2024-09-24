// Libraries for OLED display
// #include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Libraries for temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin Definitions
#define TEMPERATURE_SENSOR_PIN 1  // Pin number for the OneWire temperature sensor
#define SPEAKER_PIN 5             // Pin number for the speaker
#define BUTTON_PIN 6              // Pin number for the button
#define ANALOG_SENSOR_PIN A2      // Pin number for the analog temperature sensor
#define RELAY_PIN 7               // Pin number for the relay

// Initialize OneWire and DallasTemperature objects
OneWire oneWire(TEMPERATURE_SENSOR_PIN);
DallasTemperature temperatureSensor(&oneWire);

// OLED Display Configuration
#define SCREEN_I2C_ADDRESS 0x3C  // I2C address of the OLED display
#define SCREEN_WIDTH 128         // Width of the OLED display in pixels
#define SCREEN_HEIGHT 64         // Height of the OLED display in pixels
#define OLED_RESET_PIN -1        // Reset pin for OLED (-1 if not used)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

// Global Variables
float currentTemperature = 0;                               // Holds the current temperature reading
const int activationTemperature = 4;                        // Temperature at which the relay is activated
float deactivationTemperature;                              // Temperature at which the relay is deactivated

int analogReading = 0;                                      // Raw analog reading from the analog sensor
int previousAnalogReading = 0;                              // Previous analog reading for comparison
int currentAnalogReading = 0;                               // Current analog reading for setting temperature

bool screenTriggerResistor = false;                         // Flag to trigger display update from resistor change
bool screenTriggerButton = false;                           // Flag to trigger display update from button press
bool RELAY_PIN_FLAG = false;

String temperatureString;                                   // Temperature value converted to string for display

unsigned long previousMillis = 0;                           // Time of the last update
unsigned long currentMillis;                                // Current time for comparison

void setup() {
  // Serial.begin(9600);                                    // Uncomment for debugging
  pinMode(RELAY_PIN, OUTPUT);                               // Set relay pin as output
  pinMode(SPEAKER_PIN, OUTPUT);                             // Set speaker pin as output
  pinMode(BUTTON_PIN, INPUT);                               // Set button pin as input

  temperatureSensor.begin();                                // Initialize the temperature sensor

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDRESS);  // Initialize the OLED display
  display.dim(1);                                           // Set display brightness (0 = max, 1 = dim)
  display.clearDisplay();                                   // Clear the display buffer
  display.setTextColor(WHITE);                              // Set text color to white
  display.setRotation(0);                                   // Set display rotation
  display.setTextWrap(false);                               // Disable text wrapping

  digitalWrite(RELAY_PIN, LOW);                             // Ensure relay is off initially

  powerOn();                                                // Play power-on melody
  display.dim(0);                                           // Set display brightness to normal
}

void loop() {
  currentMillis = millis();  // Update current time

  updateTemperatureEveryTenSeconds();  // Update temperature reading every 10 seconds
  handleTemperatureSettings();         // Handle temperature settings and display updates
  updateTemperatureBasedOnSensor();    // Update temperature settings based on sensor readings
  controlRelay();                      // Control the relay based on temperature
}

// Function to update temperature reading every 10 seconds
void updateTemperatureEveryTenSeconds() {
  if (currentMillis - previousMillis > 10000) {  // Check if 10 seconds have passed
    readTemperature();                           // Read current temperature
    previousMillis = currentMillis;              // Update last update time
  }
}

// Function to handle temperature settings and display updates
void handleTemperatureSettings() {
  if (screenTriggerResistor) {                     // Check if display update is triggered by resistor
    displayTemperatureBorders();                   // Display temperature borders
    if (currentMillis - previousMillis >= 3000) {  // Check if 3 seconds have passed
      screenTriggerResistor = false;               // Reset trigger flag
    }
  } else {
    if (digitalRead(BUTTON_PIN) == HIGH) {  // Check if button is pressed
      screenTriggerButton = true;           // Set button trigger flag
      previousMillis = currentMillis;       // Update last update time
    }
    if ((currentMillis - previousMillis < 3000) && screenTriggerButton) {  // Check if 3 seconds have passed since button press
      displayTemperatureBorders();                                         // Display temperature borders
    } else {
      screenTriggerButton = false;  // Reset button trigger flag
      displayCurrentTemperature();  // Display current temperature
    }
  }

  // Check if analog reading has changed to trigger a display update
  if (previousAnalogReading != currentAnalogReading) {
    previousAnalogReading = currentAnalogReading;  // Update previous analog reading
    screenTriggerResistor = true;                  // Set resistor trigger flag
    previousMillis = currentMillis;                // Update last update time
  }
}

// Function to control the relay based on the current temperature
void controlRelay() {
  if (currentTemperature >= activationTemperature) {  // Check if temperature exceeds activation threshold
    digitalWrite(RELAY_PIN, HIGH);                    // Activate the relay
    RELAY_PIN_FLAG = true;
  }
  if (currentTemperature <= deactivationTemperature && currentTemperature != -127) {  // Check if temperature is below deactivation threshold
    digitalWrite(RELAY_PIN, LOW);                                                     // Deactivate the relay
    RELAY_PIN_FLAG = false;
  }
}

// Function to update temperature settings based on the analog sensor
void updateTemperatureBasedOnSensor() {
  analogReading = analogRead(ANALOG_SENSOR_PIN);  // Read the analog sensor value
  // Update deactivation temperature and analog reading step based on sensor value ranges
  if (analogReading >= 0 && analogReading < 120) {
    deactivationTemperature = -5;
    currentAnalogReading = 5;
  } else if (analogReading >= 120 && analogReading < 200) {
    deactivationTemperature = -4.5;
    currentAnalogReading = 4;
  } else if (analogReading >= 200 && analogReading < 700) {
    deactivationTemperature = -4;
    currentAnalogReading = 3;
  } else if (analogReading >= 700 && analogReading <= 2000) {
    deactivationTemperature = -3.5;
    currentAnalogReading = 2;
  } else if (analogReading >= 2000 && analogReading <= 3800) {
    deactivationTemperature = -3;
    currentAnalogReading = 1;
  } else if (analogReading >= 3900 && analogReading <= 4096) {
    deactivationTemperature = 0;
    currentAnalogReading = 0;
  }
}

// Play melody on power up
void powerOn() {
  for (int i = 1; i <= 3; i++) {  // Play three short tones
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(80);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(50);
  }
  delay(80);
  for (int i = 1; i <= 3; i++) {  // Play three longer tones
    digitalWrite(SPEAKER_PIN, HIGH);
    delay(80);
    digitalWrite(SPEAKER_PIN, LOW);
    delay(150);
  }
}

// Function to display the current temperature on the OLED screen
void displayCurrentTemperature() {
  display.clearDisplay();                                    // Clear the display buffer
  temperatureString = String(currentTemperature);            // Convert temperature to string
  temperatureString[temperatureString.length() - 1] = NULL;  // Remove last character for formatting

  // Center the temperature string on the display
  display.setCursor((SCREEN_WIDTH - (temperatureString.length() - 1) * 24) / 2, (SCREEN_HEIGHT - 28) / 2);
  display.setTextSize(4, 4);         // Set text size
  if (currentTemperature == -127){
    display.setTextSize(2, 2);
    display.setCursor(7, 7);
    display.print("SensorERR");
  }
  display.print(temperatureString);  // Print the temperature string
  if (RELAY_PIN_FLAG){
    display.setTextSize(1);
    display.setCursor(90, 3);
    display.print("Active");
  }
  if (!RELAY_PIN_FLAG){
    display.setTextSize(1);
    display.setCursor(90, 3);
    display.print("Sleep");
  }
  display.display();                 // Update the display
}

// Function to read the temperature from the sensor
void readTemperature() {
  temperatureSensor.requestTemperatures();                    // Request temperature readings from the sensor
  currentTemperature = temperatureSensor.getTempCByIndex(0);  // Get temperature in Celsius
}

// Function to display temperature borders (high and low thresholds) on the OLED screen
void displayTemperatureBorders() {
  display.clearDisplay();  // Clear the display buffer

  display.setTextSize(2, 3);  // Set text size for labels
  display.setCursor(7, 7);
  display.println("High");  // Label for activation temperature

  display.setCursor(7, 38);
  display.println("Low");  // Label for deactivation temperature

  display.setCursor(110, 7);
  display.println(activationTemperature);  // Display activation temperature

  if (deactivationTemperature == 0) {
    display.setCursor(110, 38);
    display.println(deactivationTemperature, 0);  // Display deactivation temperature with no decimal places
  } else {
    display.setCursor(74, 38);
    display.println(deactivationTemperature, 1);  // Display deactivation temperature with one decimal place
  }

  display.display();  // Update the display
}
