// #include <QuickPID.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#include <PID_v1.h>
#include "Adafruit_SHT31.h" 
#include "driver/mcpwm.h" 
 
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
