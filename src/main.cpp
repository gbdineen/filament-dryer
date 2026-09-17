/********************************************************
   PID Basic Example
   Reading analog pidInput 0 to control analog PWM pidOutput 3
 ********************************************************/

// #include <QuickPID.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#include <PID_v1.h>
#include "Adafruit_SHT31.h" 
// #include "driver/mcpwm.h"
// #include "tempSensors.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
// #include "driver/mcpwm_prelude.h"

// #define PIN_OUTPUT 8
#define SEESAW_ADDR 0x36
#define SS_SWITCH 24
#define SS_NEOPIX  6

#define ONE_WIRE_BUS 1
#define WIRE Wire

OneWire oneWire(ONE_WIRE_BUS);

int SDA_0 = 5;
int SCL_0 = 6;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);
int32_t encoder_position;

bool enableHeater = true;
uint8_t loopCnt = 0;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

DallasTemperature sensors(&oneWire);

double pidSetpoint, pidInput, pidOutput;
// double Kp = 0.49460, Ki = 0.00487, Kd =  1.56301;
double Kp = 16;
double Ki = 0;
double Kd = 0;
PID myPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);

int WindowSize = 5000;
unsigned long windowStartTime;

// Pin definitions
const int PWM_PIN = 2;   // D2 on XIAO = GPIO 3
const int POT_PIN = A0;  // A0 on XIAO = GPIO 26
const int LED_PIN = 21;

// PWM settings
const int PWM_FREQ = 5000;    // 25 kHz frequency for computer fans
const int PWM_RESOLUTION = 8;  // 8-bit resolution (0-255)


uint32_t Wheel(byte WheelPos) { 
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return sspixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return sspixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return sspixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void setup()
{
  WIRE.begin(SDA_0, SCL_0);

  Serial.begin(115200);
  while (!Serial) delay(10);

  windowStartTime = millis();
  pidSetpoint = 60;

  ledcAttach(PWM_PIN, PWM_FREQ, PWM_RESOLUTION);

  /********************************************************
   SHT
  ********************************************************/
  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  // Serial.print("Heater Enabled State: ");
  // if (sht31.isHeaterEnabled())
  //   Serial.println("ENABLED");
  // else
  //   Serial.println("DISABLED");

  /********************************************************
   PID 
  ********************************************************/


  // 1. CRITICAL FOR ANTI-WINDUP: Set limits matching your ESP32 PWM resolution.
  // For standard 8-bit PWM (0-255). For 10-bit ESP32 ledc, use (0, 1023).
  myPID.SetOutputLimits(0, 255);
  
  // myPID.SetSampleTime(5000); 
  // turn the PID on
  myPID.SetMode(AUTOMATIC);

   /********************************************************
   THERMISTOR
  ********************************************************/
  sensors.begin();
}

void loop()
{
  /********************************************************
   SHT
  ********************************************************/
  uint16_t t = sht31.readTemperature();
  // float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    // Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  // if (! isnan(h)) {  // check if 'is not a number'
  //   // Serial.print("Hum. % = "); Serial.println(h);
  // } else { 
  //   Serial.println("Failed to read humidity");
  // }

  // delay(1000);

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  // if (loopCnt >= 30) {
  //   enableHeater = !enableHeater;
  //   sht31.heater(enableHeater);
  //   Serial.print("Heater Enabled State: ");
  //   if (sht31.isHeaterEnabled())
  //     Serial.println("ENABLED");
  //   else
  //     Serial.println("DISABLED");

  //   loopCnt = 0;
  // }
  // loopCnt++;
  

  /********************************************************
   THERMISTOR
  ********************************************************/
  // Send the command to get temperatures from all sensors on the bus
  // sensors.requestTemperatures(); 

  // // // Fetch the temperature in Celsius for the first sensor (index 0)
  // float tempC = sensors.getTempCByIndex(0);
  // float tempF;

  // // // Check if the reading is valid before printing
  // if(tempC != DEVICE_DISCONNECTED_C) {
  //   // Serial.print("Temperature: ");
  //   // Serial.print(tempC);
  //   // Serial.print(" °C  |  ");
    
  //   // Convert Celsius to Fahrenheit
  //   // float tempF = DallasTemperature::toFahrenheit(tempC);
  //   // Serial.print(tempF);
  //   // Serial.println(" °F");
  // } else {
  //   Serial.println("Error: Could not read temperature data. Check connections.");
  // }

  pidInput = t;
  // pidInput = tempC;
  myPID.Compute();

  ledcWrite(PWM_PIN, int(pidOutput));

  Serial.print(">");
  Serial.print("pidInput:");Serial.print(t);Serial.print(",");
  Serial.print("pidOutput:");Serial.print(int(pidOutput));Serial.print(",");
  Serial.print("setPoint:");Serial.println(pidSetpoint);

  delay(500);

}
