///PLANT'ONATOR CODE
#include <Arduino.h>
#include <Adafruit_AHT10.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>     // include Adafruit graphics library
#include <Adafruit_SSD1306.h> // include Adafruit SSD1306 OLED display driver

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_AHT10 aht;

float temperature;
float humidity;
float moisture_percentage;
unsigned long oledScreenViewTime = 2000;
unsigned long checkDataPrevMillis;
bool soilMoistureViewActive = false;
int irrigationPin = 12;
bool irrigationCurrentState;

const int sensor_pin = A0;  /* Connect Soil moisture analog sensor pin to A0 of NodeMCU */

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

void updateOled(float temperature, float humidity, float moisture_percentage) {
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
    display.print("Soil Moisture (Percen.)");
    display.setTextSize(2);
    display.setCursor(0, 10);
    display.print(moisture_percentage);
    display.print(" %");
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

  Serial.print("Soil Moisture(in Percentage) = ");
  Serial.print(moisture_percentage);
  Serial.println("%");

  delay(100);
  return moisture_percentage;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize sensors
  initOled();
  initAHT();
  pinMode(irrigationPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  moisture_percentage = getMoisturePercentage();
  if (moisture_percentage > 50) {
  digitalWrite(irrigationPin, HIGH);
  Serial.println("IrrigationStatePin changed to HIGH");

  }
  else {
  digitalWrite(irrigationPin, LOW);
  Serial.println("IrrigationStatePin changed to LOW");
  }

  updateOled(temp.temperature, humidity.relative_humidity, moisture_percentage);

  if ((millis() - checkDataPrevMillis > oledScreenViewTime || checkDataPrevMillis == 0))
  {
    checkDataPrevMillis = millis(); 
    soilMoistureViewActive = !soilMoistureViewActive;

  }
}
