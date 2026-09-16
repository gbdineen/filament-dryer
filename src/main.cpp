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
#include "driver/mcpwm.h"
// #include "driver/mcpwm_prelude.h"

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

#define PWM_OUTPUT_PIN 7

// Setup step-up and step-down tracking
float dutyCycle = 0.0;
bool increasing = true;
 int activeState;

int SDA_0 = 5;
int SCL_0 = 6;
Adafruit_seesaw ss;
seesaw_NeoPixel sspixel = seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800);
int32_t encoder_position;

// Define Variables we'll be connecting to
double pidSetpoint, pidInput, pidOutput;



// Specify the links and initial tuning parameters
double Kp = 0.49460, Ki = 0.00487, Kd =  1.56301;
// double Kp = 0.0, Ki = 0.0, Kd = 1.0;
PID myPID(&pidInput, &pidOutput, &pidSetpoint, Kp, Ki, Kd, DIRECT);
Thermistor* thermistor;

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

const int freq = 50 ;      // PWM frequency in Hertz (5 kHz)
const int channel = 0;      // LEDC channel (0-7 on ESP32-S3)
const int resolution = 8;   // Resolution in bits (8-bit = values from 0 to 255)

// int dutyCycle;

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

   // Initialize the MCPWM GPIO for Unit 0, Operator A
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PWM_OUTPUT_PIN);

    // Configure MCPWM settings
  mcpwm_config_t pwm_config;
  pwm_config.frequency = 1000;             // Frequency = 1kHz
  pwm_config.cmpr_a = 0;                  // Initial duty cycle for PWM0A = 0%
  pwm_config.cmpr_b = 0;                  // Initial duty cycle for PWM0B = 0%
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // Active HIGH puls

    // Initialize MCPWM Unit 0, Timer 0 with our configuration
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
  
  // Format headers for Arduino IDE 2.x Serial Plotter
  Serial.println("Duty_Cycle_Pct,Live_PWM_State");


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

  
  pidSetpoint = 60.0;
  // initialize the variables we're linked to
  // pidInput = analogRead(PIN_INPUT);

  // 1. CRITICAL FOR ANTI-WINDUP: Set limits matching your ESP32 PWM resolution.
  // For standard 8-bit PWM (0-255). For 10-bit ESP32 ledc, use (0, 1023).
  // myPID.SetOutputLimits(0, WindowSize);
  
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


  if (increasing) { 
    dutyCycle += 1.0;
    if (dutyCycle >= 100.0) increasing = false;
  } else {
    dutyCycle -= 1.0;
    if (dutyCycle <= 0.0) increasing = true;
  }

    //   Serial.print(">");
    // // Serial.print("dutyCycle:"); Serial.print(dutyCycle); Serial.print(",");
    // // Serial.print("pidOutput:"); Serial.print(pidOutput); Serial.print(",");
    // Serial.print("pidOutput:"); Serial.println(pidOutput);
    // Serial.print("activeState:"); Serial.println(activeState);

  // Apply the updated duty cycle to MCPWM Unit 0, Timer 0, Operator A
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dutyCycle);
   Serial.print(mcpwm_read)

    long result = map(pidOutput, 0, 60, 0, 255);

    // mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, result);



    for (int i = 0; i < 20; i++) {
         int activeState = (i < (dutyCycle / 5.0)) ? 100 : 0;

      Serial.print(">");
      Serial.print("dutyCycle:"); Serial.print(dutyCycle); Serial.print(",");
      // Serial.print("pidOutput:"); Serial.print(pidOutput); Serial.print(",");
      Serial.print("activeState:"); Serial.println(activeState);
      delay(2);
    }
    // Generate a visualization of the high/low state based on loop timing
  // for (int i = 0; i < 20; i++) {
  //   // Generate a pseudo-square wave relative to the current duty cycle
  //   activeState = (i < (pidOutput / 5.0)) ? 100 : 0;
  //         // Print values in comma-separated format for the Serial Plotter
  //   // Serial.print(">");
  //   // // Serial.print("dutyCycle:"); Serial.print(dutyCycle); Serial.print(",");
  //   // Serial.print("pidOutput:"); Serial.print(pidOutput); Serial.print(",");
  //   // Serial.print("activeState:"); Serial.println(activeState);

  // }



    // delay(1000);

}
