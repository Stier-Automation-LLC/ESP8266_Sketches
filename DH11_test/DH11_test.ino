
// Example testing sketch for  DHT humidity/temperature sensors
// Written by Steven Stier - Stier Automation 
// 06/06/2023 V0.0.1 Initial Release

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h> //Including library for dht

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C
//#define SCREEN_ADDRESS 0x78
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


bool ledState;
int count;


#define LEDPIN 2 //PIN WHERE LED Connected D4=gpio2

// ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
#define DHTPIN 14 //PIN WHERE DHT11 is connected D5=gpio14

// Uncomment whatever type you're using
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Connect pin 1 (+) (on the left) of the sensor to 3.3V
// Connect pin 2 (out) of the sensor to whatever your DHTPIN is
// Connect pin 3 (on the right) of the sensor to GROUND (if your sensor has 3 pins)
// Note: didnt seem to need Pull up resister on this one

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHT11 test!"));
  // initialize the OLED object
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  pinMode(LEDPIN, OUTPUT);

  dht.begin();
  
  ledState = false;
  count = 0;
}

void loop() {
  delay(2000);
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float t = dht.readTemperature(true);
  // Read the Humidity
  float h = dht.readHumidity();


  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  //prints out the Temperature and Humidity to serial Chanel
  Serial.printf("%d Temp: %.1f°F Humidity: %.0f \n",count++,t,h);  
  
  //turn the LED on and off
  ledState = !ledState;
  digitalWrite(LEDPIN, ledState);

  // Clear the buffer.
  display.clearDisplay();

  // Display Text
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.printf("Temp:%.1f",t); 
  display.setCursor(0,20);
  display.printf("Hum:%.0f\n",h); 
  display.setCursor(0,40);
  if (ledState){
     display.write(3);
   }
   

  display.display();

}
