///PLANT'ONATOR CODE
#include <Arduino.h>
#include <Adafruit_AHT10.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>     // include Adafruit graphics library
#include <Adafruit_SSD1306.h> // include Adafruit SSD1306 OLED display driver

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SOIL_MOISTURE_MIN 30

//Pin definition
int irrigationPin = 12;
int touchSensorPin = 13;
const int sensor_pin = A0;  /* Connect Soil moisture analog sensor pin to A0 of NodeMCU */

//AHT variables
float temperature;
float humidity;

//Soil humidity sensor variables
float moisture_percentage;
bool irrigationCurrentState;

//Screen variables
bool soilMoistureViewActive = false;
unsigned long oledScreenViewTime = 2500;
unsigned long checkDataPrevMillis;
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_AHT10 aht;

//Irrigation timer variables
unsigned long checkIrrigationPrevMillis;
unsigned long lastcheckIrrigationMillis;
unsigned long checkIrrigationTimer = 900000; //timer until change irrigation state to on 40 min
unsigned long irrigationTimerON = 60000; //timer until change irrigation state to on 40 min
bool checkIrrigationFlag = true;

// Arduino function that converts milliseconds into HH:MM:SS format
String msToHMS(unsigned long milliseconds) {
  int seconds = milliseconds / 1000;
  int minutes = seconds / 60;
  int hours = minutes / 60;

  String hms = String(hours) + ":";
  hms += String(minutes % 60) + ":";
  hms += String(seconds % 60);

  return hms;
}

void initOled() {
  delay(1000);  // wait a second
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  // clear the display buffer.
  display.clearDisplay();
  display.setTextSize(1);   // text size = 1
  display.setTextColor(WHITE, BLACK);  // set text color to white and black background
  display.setTextWrap(false);           // disable text wrap
}

void sendNotificationOled(String message)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.print(message);
  display.display();
  delay(2000);

}

void updateOled(float temperature, float humidity, float moisture_percentage, bool irrigationCurrentState) {
  if (soilMoistureViewActive == false) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Temperature");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(temperature);
    display.print(" ");
    display.setTextSize(1);
    display.write(167);
    display.setTextSize(2);
    display.print("C ");
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Humidity");
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(humidity);
    display.print(" % ");
    display.display();
  }
  else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("SoilMoist - min= ");
       display.print(SOIL_MOISTURE_MIN);
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(moisture_percentage);
    display.print(" %");
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("CurrentStatus: ");
    display.print(irrigationCurrentState);
    display.setTextSize(1);
    display.setCursor(0, 50);
    //If it is irrigating (irrigationCurrentState==0) shows the remaining time to stop irrigation
    if (!irrigationCurrentState) {
      display.print("Irrigating: ");
      display.print(msToHMS(lastcheckIrrigationMillis + irrigationTimerON - millis()));
    }
    //If it not irrigating (irrigationCurrentState==1) shows the remaining time to check moisture_percentage level
    else {
      display.print("TimeToCheck: ");
      display.print(msToHMS(checkIrrigationPrevMillis + checkIrrigationTimer - millis()));
    }
    display.display();
  }
}

void initAHT()
{
  if (!aht.begin())
  {
    Serial.println("Could not find AHT10? Check wiring");
    while (1)
      delay(10);
  }
  Serial.println("AHT10 found");
}

float getMoisturePercentage() {
  float moisture_percentage;
  moisture_percentage = ( 100.00 - ( (analogRead(sensor_pin) / 1023.00) * 100.00 ) );

  //Serial.print("Soil Moisture(in Percentage) = ");
  //Serial.print(moisture_percentage);
  //Serial.println("%");
  delay(100);
  return moisture_percentage;
}

void changeIrrigationState(bool irrigationCurrentState)
{
  bool irrigationState;
  if (irrigationCurrentState) {
    irrigationState = 0;
  }
  else {
    irrigationState = 1;
  }
  digitalWrite(irrigationPin, irrigationState);
  Serial.println("IrrigationStatePin changed to: ");
  Serial.println(irrigationState);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  // Initialize sensors
  initOled();
  initAHT();
  pinMode(irrigationPin, OUTPUT);
  pinMode(touchSensorPin, INPUT);
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  moisture_percentage = getMoisturePercentage();
  irrigationCurrentState = digitalRead(irrigationPin);
  updateOled(temp.temperature, humidity.relative_humidity, moisture_percentage, irrigationCurrentState);

  if (digitalRead(touchSensorPin)) {
    delay(200);
    changeIrrigationState(digitalRead(irrigationPin));
  }

  //Timer to check change between th einfor that is show in the screen
  if ((millis() - checkDataPrevMillis > oledScreenViewTime || checkDataPrevMillis == 0))
  {
    checkDataPrevMillis = millis();
    soilMoistureViewActive = !soilMoistureViewActive;

  }

  //Timer to check irrigation status to avoid overwatering on slow response
  if ((millis() - checkIrrigationPrevMillis > checkIrrigationTimer ))
  {
    Serial.println("Checking Irrigation");
    checkIrrigationPrevMillis = millis();
    checkIrrigationFlag = 1;
  }

  //Si no está regando pero el soilHumidity es menor que el esperado cambia el estado de irrigación a encendido
  if ( irrigationCurrentState == 1 && moisture_percentage < SOIL_MOISTURE_MIN && checkIrrigationFlag)
  {
    Serial.println("Starting irrigation");
    lastcheckIrrigationMillis = millis();
    checkIrrigationFlag = 0;
    changeIrrigationState(irrigationCurrentState);
  }
  //Si está regando comprueba el tiempo desde que empezó el riego y cuando sea mayor lo apaga
  else if (irrigationCurrentState == 0 && (millis() - lastcheckIrrigationMillis >= irrigationTimerON) )
  {
    Serial.println("Turning off irrigation due timout");
    lastcheckIrrigationMillis = millis();
    checkIrrigationFlag = 0;
    changeIrrigationState(irrigationCurrentState);
  }

}
