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
#include <Thermistor.h>
#include <NTC_Thermistor.h>

#define PIN_INPUT 7
#define PIN_OUTPUT 8
#define WIRE Wire
#define SEESAW_ADDR 0x36
#define SS_SWITCH 24
#define SS_NEOPIX  6

// Thermistor Parameters (Standard 3D Printer / Heater NTC 100K 3950 setup)
#define SENSOR_RESISTANCE   100000  // 100k Ohm Thermistor at 25 degrees C
#define REFERENCE_RESISTANCE 10000  // 10k Ohms (Your two 5k resistors in series)
#define B_COEFFICIENT       3950    // The beta factor of your thermistor
#define NOMINAL_TEMPERATURE 25      // Temperature for nominal resistance

// ESP32 ADC Resolution: 12-bit (0 to 4095)
#define ESP32_ADC_RESOLUTION 4095

int SDA_0 = 5;
int SCL_0 = 6;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);
int32_t encoder_position;

// Define Variables we'll be connecting to
double pidSetpoint, pidInput, pidOutput;


  // kp: 0.49460
  //     ki: 0.00487
  //     kd: 12.56301

// Specify the links and initial tuning parameters
// double Kp = 0.49460, Ki = 0.00487, Kd =  1.56301;
double Kp = 3.0, Ki = 0.05, Kd = 1.0;
PID myPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);
Thermistor* thermistor;

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

const int freq = 50 ;      // PWM frequency in Hertz (5 kHz)
const int channel = 0;      // LEDC channel (0-7 on ESP32-S3)
const int resolution = 8;   // Resolution in bits (8-bit = values from 0 to 255)

int dutyCycle;

int WindowSize = 500;
unsigned long windowStartTime;


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

  // pinMode(PIN_OUTPUT, OUTPUT); 
  // pinMode(PWM_PIN, OUTPUT);




  /********************************************************
   Rotary Encoder
 ********************************************************/

  // Serial.println("Looking for seesaw!");
  
  // if (! ss.begin(SEESAW_ADDR) || ! sspixel.begin(SEESAW_ADDR)) {
  //   Serial.println("Couldn't find seesaw on default address");
  //   while(1) delay(10);
  // }
  // Serial.println("seesaw started");

  // // set not so bright!
  // sspixel.setBrightness(20);
  // sspixel.show();
  
  // // use a pin for the built in encoder switch
  // ss.pinMode(SS_SWITCH, INPUT_PULLUP);

  // // get starting position
  // encoder_position = ss.getEncoderPosition();

  // Serial.println("Turning on interrupts");
  // delay(10);
  // ss.setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  // ss.enableEncoderInterrupt();

  /// SHT

  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");

  /********************************************************
   PID
  ********************************************************/
  // analogSetAttenuation(ADC_2_5db);
  // analogSetWidth(12); 

  // thermistor = new NTC_Thermistor(
  //   PIN_INPUT, 
  //   REFERENCE_RESISTANCE, 
  //   SENSOR_RESISTANCE, 
  //   NOMINAL_TEMPERATURE, 
  //   B_COEFFICIENT, 
  //   ESP32_ADC_RESOLUTION
  // );

  //    // 1. Configure LEDC channel functionality
  // ledcSetup(channel, freq, resolution);
  
  // // 2. Attach the channel to the specified GPIO pin
  // ledcAttachPin(PIN_OUTPUT, channel);
  
  pidSetpoint = 60.0;
  // initialize the variables we're linked to
  // pidInput = analogRead(PIN_INPUT);

  // 1. CRITICAL FOR ANTI-WINDUP: Set limits matching your ESP32 PWM resolution.
  // For standard 8-bit PWM (0-255). For 10-bit ESP32 ledc, use (0, 1023).
  myPID.SetOutputLimits(0, WindowSize);
  
  // myPID.SetSampleTime(500); 
  // turn the PID on
  myPID.SetMode(AUTOMATIC);
}

void loop()
{
  ///// SHT
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }

  // delay(1000);

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  if (loopCnt >= 30) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sht31.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");

    loopCnt = 0;
  }
  loopCnt++;

  // pidInput = analogRead(PIN_INPUT);
  pidInput = t;
  myPID.Compute();
  // analogWrite(PIN_OUTPUT, pidOutput);
  // ledcWrite(channel, pidOutput);
  // ledcWrite(0, pidOutput);
  // analogWrite(PIN_OUTPUT, pidOutput);

    /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
  // if (millis() - windowStartTime > WindowSize)
  // { //time to shift the Relay Window
  //   windowStartTime += WindowSize;
  // }
  // if (pidOutput < millis() - windowStartTime) digitalWrite(PIN_OUTPUT, HIGH);
  // else digitalWrite(PIN_OUTPUT, LOW);

  // Serial.println(ledcRead(PIN_OUTPUT));

  digitalWrite(PIN_OUTPUT, pidOutput);

    // Plotter outputSD
  Serial.print(">");
  Serial.print("Setpoint:");   Serial.print(pidSetpoint); Serial.print(",");
  Serial.print("CurrentTemp:");Serial.print(pidInput);    Serial.print(",");
  // Serial.print("PWM_OUTPUT:"); Serial.println(ledcRead(0));
  // Serial.print("PWM:"); Serial.println(pidOutput);
   Serial.print("PWM:"); Serial.println(digitalRead(PIN_OUTPUT));




  delay(500);
  // Input = analogRead(PIN_INPUT);
  // myPID.Compute();
  // analogWrite(PIN_OUTPUT, Output);
}

// void loop()
// {
//   /********************************************************
//    Rotary Encoder
//   ********************************************************/
//    if (! ss.digitalRead(SS_SWITCH)) {
//     Serial.println("Button pressed!");
//   }

//   int32_t new_position = ss.getEncoderPosition();
//   // did we move arounde?
//   if (encoder_position != new_position) {
//     Serial.println(new_position);         // display new position

//     // pidInput = new_position;

//     // change the neopixel color
//     sspixel.setPixelColor(0, Wheel(new_position & 0xFF));
//     sspixel.show();
//     encoder_position = new_position;      // and save for next round
//   }

//   /********************************************************
//    PID
//   ********************************************************/

//     // Read raw 12-bit ADC (0 to 4095) from Pin 7
//   // int rawADC = analogRead(PIN_INPUT);
//   uint32_t adcSum = 0;
//   for(int i = 0; i < 64; i++) {
//     adcSum += analogRead(PIN_INPUT);
//   }
//   int rawADC = adcSum / 64; // The averaged, clean signal

//   // Safeguard: Prevent division-by-zero errors if the sensor unplugs
//   if (rawADC >= 4095) rawADC = 4094;
//   if (rawADC <= 0) rawADC = 1;

//   // Calculate the exact real-time resistance of your 100k Thermistor
//   // Formula based on your 10k Ohm series divider reference resistor
//   double thermistorResistance = REFERENCE_RESISTANCE * ((4095.0 / (double)rawADC) - 1.0);

//   // Apply the Steinhart-Hart Beta Equation to solve for Kelvin
//   double kelvin = thermistorResistance / (double)SENSOR_RESISTANCE; // (R/Ro)
//   kelvin = log(kelvin);                                            // ln(R/Ro)
//   kelvin /= (double)B_COEFFICIENT;                                 // 1/B * ln(R/Ro)
//   kelvin += 1.0 / ((double)NOMINAL_TEMPERATURE + 273.15);          // + (1/To)
//   kelvin = 1.0 / kelvin;                                           // Invert to get Kelvin

//   // Convert Kelvin down to real Celsius
//   double realCelsius = kelvin - 273.15;

//   // Pass the mathematically perfect temperature straight into your PID loop
//   pidInput = realCelsius;
  
//   myPID.Compute();
//   analogWrite(PIN_OUTPUT, pidOutput);

//   // Plotter output
//   Serial.print(">");
//   Serial.print("Setpoint:");   Serial.print(pidSetpoint); Serial.print(",");
//   Serial.print("CurrentTemp:");Serial.print(pidInput);    Serial.print(",");
//   Serial.print("MOSFET_PWM:"); Serial.println(pidOutput);
  
//   delay(10);
// }
