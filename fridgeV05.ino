    //version 0.5
// Libs for screen
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// libs for temp sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20 1
#define speaker 5
#define switcher 6
#define sensorPin A2
#define relayPin 7
#define relayPin2 9

OneWire ourWire(DS18B20);
DallasTemperature sensor(&ourWire);

#define SCREEN_I2C_ADDR 0x3C // or 0x3C
#define SCREEN_WIDTH 128     // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RST_PIN -1      // Reset pin (-1 if not available)

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST_PIN);

float temperature = 0;
bool beeperActivator = false;                     // this is like a trigger that makes speaker active only when it's true;

const float hysteresis = 0.2;
const int actTemperature = 5;
float deactTemperature;

int analogInValue = 0;
int analogInValueInSteps1;
int analogInValueInSteps2;

bool scereenTrigger = false;

String temperatureInString;                      // I convert the float into the temperatureInString var
String temperatureHeader = "My temperature";

unsigned long previousMillis = 0; 
unsigned long currentMillis;


void setup() {
  // Serial.begin(9600);                // for debugging
  // Serial.println("start");
  pinMode(relayPin, OUTPUT);
  pinMode(speaker, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(switcher, INPUT);

  sensor.begin();

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDR);
  display.dim(1);  //Set brightness (0 is maximun and 1 is a little dim)+
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.setTextWrap(false);
  
  digitalWrite(relayPin, LOW);

  powerOn();
  display.dim(0);
}

void loop() {
  currentMillis = millis();
  getTemperatureEachTenSec();
  speakerTrigger();        
  temperatureSetRoutine();
  beeperReactionHigh();
  beeperReactionLow();
  temperatureSwithcer();
  relayTrigger();

  // Serial.println(analogInValue);
  // delay(100);
}
//_____________________________________________________________________________________________________________________________________________________________

void beeperReactionLow(){
  if (temperature <= deactTemperature && beeperActivator == true){
    speakerReactionLow();
    beeperActivator = false;
  }
}

void beeperReactionHigh(){
  if (temperature >= actTemperature && beeperActivator == true){
    speakerReactionHigh();
    speakerReactionHigh();
    beeperActivator = false;
  }
}

void getTemperatureEachTenSec(){
  if(currentMillis - previousMillis > 10000){
    getTemperature();
    previousMillis = currentMillis;
  }
}

void temperatureSetRoutine(){
  if(scereenTrigger || (digitalRead(switcher) == HIGH)){                       //temperatureSetRoutine
    tempBorders();
    if(currentMillis - previousMillis < 3000){
    }else{
      scereenTrigger = false;
    }
  }else{
    if (digitalRead(switcher) == HIGH){
      tempBorders();
    }else{
      myTemperature();
    }
  }

  if(analogInValueInSteps1 != analogInValueInSteps2){
    analogInValueInSteps2 = analogInValueInSteps1;
    scereenTrigger = true;
    previousMillis = currentMillis;
  }
}

void relayTrigger(){
  if(temperature >= actTemperature){
    digitalWrite(relayPin, HIGH);
    digitalWrite(relayPin2, HIGH);    
  }
  if(temperature <= deactTemperature && temperature != -127){
    digitalWrite(relayPin, LOW);
    digitalWrite(relayPin2, LOW);    
  }
}

void temperatureSwithcer(){
  analogInValue = analogRead(sensorPin);
  if(analogInValue >= 0 && analogInValue < 120){
    deactTemperature = -5;
    analogInValueInSteps1 = 5;
  }else if(analogInValue >= 120 && analogInValue < 200){
    deactTemperature = -4.5;
    analogInValueInSteps1 = 4;  
  }else if(analogInValue >= 200 && analogInValue < 700){
    deactTemperature = -4;
    analogInValueInSteps1 = 3;  
  }else if(analogInValue >= 700 && analogInValue <= 2000){
    deactTemperature = -3.5;
    analogInValueInSteps1 = 2;  
  }else if(analogInValue >= 2000 && analogInValue <= 3800){
    deactTemperature = -3;
    analogInValueInSteps1 = 1;  
  }else if(analogInValue >= 3900 && analogInValue <= 4096){
    deactTemperature = 0;
    analogInValueInSteps1 = 0;  
  }
}

void powerOn(){                         // The function that plays ones when power is on. It plays melody.
    for(int i1 = 1; i1 <= 3; i1++){
    digitalWrite(speaker, HIGH);
    delay(80);
    digitalWrite(speaker, LOW);
    delay(50);
    }
    delay(80);
  for(int i2 = 1; i2 <= 3; i2++){
    digitalWrite(speaker, HIGH);
    delay(80);
    digitalWrite(speaker, LOW);
    delay(150);
  }
}

void speakerReactionHigh () {         // The function that plays melody related for reaching HIGH temperature parameter
  for(int i = 1; i <= 4; i++){
    digitalWrite(speaker, HIGH);
    delay(70);
    digitalWrite(speaker, LOW);
    delay(70);
  }
  delay(1000);
}

void speakerReactionLow () {         // The function that plays melody related for reaching LOW temperature parameter
  digitalWrite(speaker, HIGH);
  delay(500);
  digitalWrite(speaker, LOW);
  delay(100);
  digitalWrite(speaker, HIGH);
  delay(70);
  digitalWrite(speaker, LOW);
  delay(1000);
}

void myTemperature(){
  display.clearDisplay();
  temperatureInString = String(temperature);
  // int length = temperatureHeader.length();
  // int length2 = temperatureInString.length() - 1;
  temperatureInString[temperatureInString.length()-1] = NULL;
	display.setCursor((128-(temperatureHeader.length())*6)/2, 10); //(x,y)        // this sets the cursor so that the input is centered in the screen
  display.setTextSize(1,2);
	display.println("My temperature");

	display.setCursor((128-(temperatureInString.length() - 1)*12)/2, 32);         // this sets the cursor so that the input is centered in the screen
  display.setTextSize(2);
	display.print(temperatureInString);
  display.display();
};

void getTemperature(){
  sensor.requestTemperatures();
  temperature = sensor.getTempCByIndex(0);
}

void tempBorders(){
  display.clearDisplay();

  display.setTextSize(1,2);
  display.setCursor(7, 2);
  display.println("Operation threshold");

  display.setTextSize(2);
  display.setCursor(7, 28);  
  display.println("Hihg");

  display.setCursor(7, 48);   
  display.println("Low");

  display.setCursor(110, 28);  
  display.println(actTemperature);

  if(deactTemperature == 0){
    display.setCursor(110, 48);
    display.println(deactTemperature , 0);

  }else{
    display.setCursor(74, 48);            //(98, 48)
    display.println(deactTemperature , 1);
  } 

  display.display();
}

void speakerTrigger(){
  if (temperature < (actTemperature - hysteresis) && temperature > (deactTemperature + hysteresis ) ){
    beeperActivator = true;
  }   
}

