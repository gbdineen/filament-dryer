#include <OneWire.h>
#include <DallasTemperature.h>
#include "Adafruit_SHT31.h" 
#include <Wire.h>

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 1
#define WIRE Wire

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// int SDA_0 = 5;
// int SCL_0 = 6;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {

  WIRE.begin(SDA_0, SCL_0);
  // Start the serial monitor
  Serial.begin(115200);
  Serial.println("DS18B20 Temperature Sensor Reading");

  /********************************************************
   SJT31
  ********************************************************/

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
   THERMISTOR
  ********************************************************/
  sensors.begin();
}

void loop() {
  
  /********************************************************
   SJT31
  ********************************************************/
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();


  if (! isnan(t)) {  // check if 'is not a number'
    // Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    // Serial.print("Hum. % = "); Serial.println(h);
  } else { 
    Serial.println("Failed to read humidity");
  }
  
  /********************************************************
   THERMISTOR
  ********************************************************/
  // Send the command to get temperatures from all sensors on the bus
  sensors.requestTemperatures(); 

  // // Fetch the temperature in Celsius for the first sensor (index 0)
  float tempC = sensors.getTempCByIndex(0);
  float tempF;

  // // Check if the reading is valid before printing
  if(tempC != DEVICE_DISCONNECTED_C) {
    // Serial.print("Temperature: ");
    // Serial.print(tempC);
    // Serial.print(" °C  |  ");
    
    // Convert Celsius to Fahrenheit
    // float tempF = DallasTemperature::toFahrenheit(tempC);
    // Serial.print(tempF);
    // Serial.println(" °F");
  } else {
    Serial.println("Error: Could not read temperature data. Check connections.");
  }

  // Serial.print("THERM °C: ");Serial.print(tempC);
  // Serial.print(" SHT °C: ");Serial.println(t);

  Serial.print(">");
  Serial.print("SHT_Temp_C:"); Serial.print(t);Serial.print(",");
  Serial.print("TERM_Temp_C:"); Serial.println(tempC);

  // Wait 2 seconds before taking the next reading
  delay(500);
}
