#include <Servo.h>
#include <DHT.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

// constants won't change. Used here to set a pin numbers :
const int eye1Pin =  2;      // the number of the Eye 1 LED pin
const int servoPin = 3;  //number of the Pin that holds the servo
const int eye2Pin =  4;      // the number of the Eye 2 LED pin
const int watchPin = 5;     // led which blinks when ever the program is running
const int buzzerPin =  6;      // the number of the pin the Buzzer is connected to
const int proxPin = 7; //number of the Pin that holds the prox switch
const int offPin = 8; //Number of the pin that holds the on/off swich
const int DHTPIN = 10; //Number of the pin that holds the on/off swich

#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);


int moisturePin = A0;    // select the input pin for the potentiometer
int moistureValue = 0;  // variable to store the value coming from the sensor
// Variables will change :
int led1State = LOW;             // ledState used to set the Eye 1 LED
int led2State = HIGH;  
int ledWatchState = LOW;
unsigned long previousMillis = 0;        // will store last time LED was updated
unsigned long previousWatchMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 1000;           // interval at which to blink Eyes (milliseconds)
const long watchInterval = 500;      // interval at which to watchdog LED (milliseconds)

void setup() {
  // put your setup code here, to run once:

  // set the digital pin as output:
  pinMode(watchPin, OUTPUT);
  pinMode(eye1Pin, OUTPUT);
  pinMode(eye2Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(proxPin, INPUT);
  pinMode(offPin, INPUT);
  myservo.attach(servoPin);  // attaches the servo on pin 9 to the servo object

  Serial.begin(9600);  
  dht.begin();
  

}

void loop() {

  // check to see if it's time to blink the LED; that is, if the
  // difference between the current time and last time you blinked
  // the LED is bigger than the interval at which you want to
  // blink the LED.

 unsigned long currentWatchMillis = millis();
  if (currentWatchMillis - previousWatchMillis >= watchInterval) {
    // save the last time you blinked the LED
    previousWatchMillis = currentWatchMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledWatchState == LOW) {
      ledWatchState = HIGH;
    } else {
        ledWatchState = LOW;
      }
 // set the LED with the ledState of the variable:
    digitalWrite(watchPin, ledWatchState);

 // check the temp and humididty sensors
 // read the value from the sensor:
  moistureValue = analogRead(moisturePin); 

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  //if (isnan(h) || isnan(f)) {
  //  Serial.println("Failed to read from DHT sensor!");
  //  return;
  //}

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  Serial.print("moisture=" );                       
  Serial.print(moistureValue);   
  Serial.print("\t");        
  Serial.print("Humidity=");
  Serial.print(h);
  Serial.print("\t");
  Serial.print("Temp=");
  Serial.print(f);
  Serial.print("\t");
  Serial.print("HeatIndex=");
  Serial.print(hif);
  Serial.print(" \t");
  if (digitalRead(proxPin) == HIGH)
  {Serial.print("Motion=Detected\t");}
  else
  {Serial.print("Motion=None\t");}
  if (digitalRead(offPin) == HIGH)
  {Serial.println("State=Enabled");}
  else
  {Serial.println("State=Disabled");}

// End of humidity Sensor Check
 
  }

    digitalWrite (13,digitalRead(offPin));
  // check if motion is detected
  if ((digitalRead(proxPin) == HIGH) && (digitalRead(offPin) == HIGH)) {
   
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (led1State == LOW) {
      led1State = HIGH;
      led2State = LOW;
    } else {
        led1State = LOW;
        led2State = HIGH;
      }

    // set the LED with the ledState of the variable:
    digitalWrite(eye1Pin, led1State);
    digitalWrite(eye2Pin, led2State);
    digitalWrite (buzzerPin, led1State);

 // fade in from min to max in increments of 5 points:
    for (int fadeValue = 0 ; fadeValue <= 45; fadeValue += 5) {
        // sets the value (range from 0 to 255):
        //analogWrite(buzzerPin, fadeValue);
        myservo.write(fadeValue);
        // wait for 15 Millisecs for Servo to get into position
        delay(15);
    }

    // fade out from max to min in increments of 5 points:
    for (int fadeValue = 45 ; fadeValue >= 0; fadeValue -= 5) {
      // sets the value (range from 0 to 255):
      //analogWrite(buzzerPin, fadeValue);
      myservo.write(fadeValue);
      // wait for 15 Millisecs for Servo to get into position
      delay(15);
    }
// fade in from min to max in increments of 5 points:
    for (int fadeValue = 0 ; fadeValue <= 250; fadeValue += 25) {
      // sets the value (range from 0 to 255):
      //analogWrite(buzzerPin, fadeValue);
      // wait for 15 Millisecs for Servo to get into position
      delay(30);
    }

    // fade out from max to min in increments of 5 points:
    for (int fadeValue = 250 ; fadeValue >= 0; fadeValue -= 25) {
      // sets the value (range from 0 to 255):
      //analogWrite(buzzerPin, fadeValue);
      // wait for 15 Millisecs for Servo to get into position
      delay(30);
    }


    
  }

 } else {

    digitalWrite(eye1Pin, LOW);
    digitalWrite(eye2Pin, LOW);
    digitalWrite(buzzerPin, LOW);
    //analogWrite(buzzerPin, 0);
    myservo.write(0);
  } 
  
}

