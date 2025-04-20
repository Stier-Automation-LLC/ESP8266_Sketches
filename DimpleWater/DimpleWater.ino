/*
  Dimple Watering System
  Measure the water usage for Yard and Garden activites. Has flow meter with Reed
  Contact ( 1/gal) and automatic valve.

  The circuit:
      D0 = GPIO16 - Output - RedLED Gal Detected
      D1 = GPIO5  - Output - ValveOpen
      D2 = GPIO4  - Input -  FlowSignal
      D3 = GPIO0  - Output  - YellowLED - Auto Running
      D4 = GPIO2  - Output - InternalLED - blinks where program is running 

      D5 - GPIO14  - Input - AutoStart PB
      D6 - GPIO12 - Input - Open valve PB
      D7 - GPIO13 - Input -  Close valve PB
      D8 - GPIO15  - Output- GreenLED - Wifi Detected
      S3 - SD3 - GPIO10  - SCL
      S2 - SD2  - GPIO9   -  SDA

  created 4/19/2025
  by Steven Stier
*/
// Load Wi-Fi library
#include <ESP8266WiFi.h>



//GPIO Assingnments
const int valveGPIO = 5; //Valve GPIO
const int flowSignalGPIO = 4; //Flow Signal GPIO

const int redLEDGPIO = 16;  // Red LED GPIO
const int yellowLEDGPIO = 0;  // Yellow LED GPIO
const int greenLEDGPIO = 15;   // Green LED GPIO
const int internalLEDGPIO = 2;  //Internal LED GPIO

const int autoStartPBGPIO = 14;  // AUto Start PB GPIO
const int openValvePBGPIO = 12;  // Open Valve PB GPIO
const int closeValvePBGPIO = 13; // Close Valve PB GPIO

const int sclGPIO = 10;
const int sdaGPIO = 9;

const int debug = LOW;
int watchDogLEDState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
long previousHeartbeatMillis = 0; //holds the time the HeartBeat LED last blinked
const long heartBeatInterval = 1000;  // interval at which to blink Heartbeat LED (milliseconds)

unsigned long autoGalsSetpoint = 100; // how many gals to deliver at  

const long totalizerInterval = 60000;           // interval at which to update the totalzier numbers
long previoustotalizerUpdateMillis = 0;  //Holds the time the Totalzizer counts were updated

//counters for each flowmeter
unsigned long totalClicks = 0; //total number of gals From totalizer, not reset
unsigned long totalGals = 0; //total number of gals From totalizer, not reset
unsigned long autoClicks = 0; //number of gals dispensed in this autoMode.
unsigned long autoGals = 0; //number of gals dispensed in this autoMode.
unsigned long clicksInWindow=0;  // Number of clicks in gpm calculation
unsigned long galsPerMin = 0; // Calculated Flow Rate

//latches so the counter doesn't get stuck counting if the magnet stays next to the sensor
boolean SensorIsLowThisScan = false;
boolean SensorWasHighLastScan= false;

int valveCMDState = LOW; //Internally this is what we want the valve to do.
int autoModeState = LOW; //Holds the automode State

String autoModeStatetxt ="Off";
String valveStatetxt = "Open";

char test[20];

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xDF, 0x44 };  
// Only need to set this if you want to use static IPs
byte ip[] = { 192, 168, 100, 11 };   //Manual setup only
byte gateway[] = { 192, 168, 100, 1 }; //Manual setup only
byte subnet[] = { 255, 255, 255, 0 }; //Manual setup only

// Replace with your network credentials
const char* ssid     = "pugfi";
const char* password = "cupcake2016";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentETime = millis();
// Previous time
unsigned long previousETime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutETime = 2000;

/*
THis methos gets called whenever the flow meter detects a gal dispensed
*/
ICACHE_RAM_ATTR void IncrementFlow() {

    clicksInWindow++;
    totalClicks++;
    if (autoModeState == HIGH) { 
       autoClicks++;;
      } 
    totalGals = totalClicks/2;
    autoGals = autoClicks/2;
    Serial.println( "Click from From Flowmeter displensed!!! --totalGals="+ String(totalGals) + " \tClickInWindow="+ String(clicksInWindow));
}

void setup() {
  //start serial connection
  Serial.begin(9600);
  // set all the outputs
  //lalve
  pinMode(valveGPIO, OUTPUT);
  //LEDS
  pinMode(redLEDGPIO, OUTPUT); 
  pinMode(yellowLEDGPIO, OUTPUT); 
  pinMode(greenLEDGPIO, OUTPUT); 
  pinMode(internalLEDGPIO, OUTPUT); 
  //set the pushbuttons to  enable the internal pull-up resistor
  pinMode(autoStartPBGPIO, INPUT_PULLUP);
  pinMode(openValvePBGPIO, INPUT_PULLUP);
  pinMode(closeValvePBGPIO, INPUT_PULLUP);

  // Flow meter signal
  pinMode(flowSignalGPIO, INPUT);

  attachInterrupt(digitalPinToInterrupt(flowSignalGPIO), IncrementFlow, RISING);
  //I2C
  //sclGPIO ;
  //sdaGPIO ;


  //Ethernet.begin(mac); // if using DHCP
  //Ethernet.begin(mac, ip, gateway, subnet); //for manual setup

  //eServer.begin();
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  digitalWrite(greenLEDGPIO, HIGH); 
}


void loop() {
  // Get the System clock
  unsigned long currentMillis = millis();

  // Flash the internal LED every second as as a hearbeat signal
  if(currentMillis - previousHeartbeatMillis > heartBeatInterval) {
      previousHeartbeatMillis  = currentMillis;   

      // if the LED is off turn it on and vice-versa:
        if (watchDogLEDState == LOW)
          watchDogLEDState = HIGH;
        else
          watchDogLEDState = LOW;
        
        digitalWrite(internalLEDGPIO, watchDogLEDState);  
    }

  //read the pushbutton value into a variable
  int autoStartPBState = digitalRead(autoStartPBGPIO);
  int openValvePBState = digitalRead(openValvePBGPIO);
  int closeValvePBState = digitalRead(closeValvePBGPIO);
  int flowSignalState = digitalRead(flowSignalGPIO);
  

  // Open the valve when the Open Pushbutton is pressed
  if (openValvePBState == LOW) {
    digitalWrite(valveGPIO, HIGH); 
    Serial.println("Valved Open from PushButton.");
  } 
  // Close the valve when the Open Pushbutton is pressed
  if (closeValvePBState == LOW) {
    digitalWrite(valveGPIO, LOW); 
    Serial.println("Valve Close from PushButton.");
    autoModeState = LOW;
  } 

  // Auto mode is activated-PB Pressed
  if (autoStartPBState == LOW) { 
    autoModeState = HIGH;
    digitalWrite(valveGPIO, HIGH);
    Serial.println("Automode Started from Pushbutton");
    autoGals = 0;
    autoClicks = 0;
  } 

  // End the Automode when weve despendes the correct number of gals
  if ((autoGals > autoGalsSetpoint) && (autoModeState == HIGH)) {
    autoModeState = LOW;
    digitalWrite(valveGPIO, LOW); 
    Serial.println("Automode Ended.");
  } 

  // Turn on the Yellow LED when were in AUTO MODE.
  if (autoModeState == HIGH) { 
    digitalWrite(yellowLEDGPIO, HIGH);
    autoModeStatetxt = "On";
  } else{
    digitalWrite(yellowLEDGPIO, LOW);
    autoModeStatetxt = "Off";
  }


  //  Light up the green LED when we get a flow signal
  if (flowSignalState == HIGH) { 
    
    digitalWrite(redLEDGPIO, HIGH);
    } else{
    digitalWrite(redLEDGPIO, LOW);
  }

  int valveState = digitalRead(valveGPIO);

  if (valveState == LOW) {
    valveStatetxt = "Closed";
  } else {
    valveStatetxt = "Open";
  }
  

  if (debug == HIGH) {
    Serial.print("AutoStart=");
    Serial.print(autoStartPBState);
    Serial.print(",OpenPB=");
    Serial.print(openValvePBState);
    Serial.print(",CloseVPB=");
    Serial.print(closeValvePBState);
    Serial.print(",flowSignal=");
    Serial.print(flowSignalState);
    Serial.print(",autoMode=");
    Serial.print(autoModeState);
    Serial.print(",valve=");
    Serial.print(valveState);
    Serial.print(",totalGals=");
    Serial.print(totalGals); 
    Serial.print(" \tgpm=");
    Serial.print(galsPerMin);
    Serial.print(" \tautoGalsSP=");
    Serial.print(autoGalsSetpoint); 
    Serial.print(" \tautoGals=");
    Serial.print(autoGals);
    Serial.println("");

  }


  // Caluclate the GPM by simply reporting the number of gals produced in 60 secs
  if(currentMillis - previoustotalizerUpdateMillis > totalizerInterval) {
      previoustotalizerUpdateMillis = currentMillis;   
      galsPerMin = clicksInWindow/2;
      clicksInWindow = 0;
      Serial.println("GPM=" + String(galsPerMin));
  }

  // Web server stuff

 WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentETime = millis();
    previousETime = currentETime;
    while (client.connected() && currentETime - previousETime <= timeoutETime) { // loop while the client's connected
      currentETime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /auto") >= 0) {
              Serial.println("Set AutoMode from INET");
              autoModeState = HIGH;
              autoModeStatetxt = "On";
              valveStatetxt = "Open";
              digitalWrite(valveGPIO, HIGH);
              autoGals = 0;
              autoClicks = 0;
            } else if (header.indexOf("GET /valveOpen") >= 0) {
              Serial.println("Valve Open from INET");
              digitalWrite(valveGPIO, HIGH);
              valveStatetxt = "Open";
            } else if (header.indexOf("GET /valveClose") >= 0) {
              Serial.println("Valve Close from INET");
              digitalWrite(valveGPIO, LOW); 
              autoModeState = LOW;
              autoModeStatetxt = "Off";
              valveStatetxt = "Closed";
            } 
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}");
            client.println(".button3 {background-color: #008000;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Dimple Watering System</h1>");;

            client.println("<h2>Total Gals: " + String(totalGals) + " gals</h2>");
            client.println("<h2> " + String(galsPerMin) + " gpm</h2>");
            // Display valve status  
            // Get the current state of the valve


            client.println("<h2>Valve: " +valveStatetxt+ "</h2>");
            // If the output5State is off, it displays the ON button       
            if (valveStatetxt=="Closed") {
              client.println("<h2><a href=\"/valveOpen\"><button class=\"button\">Open</button></a></h2>");
            } else {
              client.println("<h2><a href=\"/valveClose\"><button class=\"button button2\">Close</button></a></h2>");
            } 
               
            // Display Auto Mode Status
            client.println("<h2>Auto Mode " + autoModeStatetxt + "</h2>");
            client.println("<h2>Setpoint: " + String(autoGalsSetpoint) + " gals</h2>");
            client.println("<h2>RunningGals: " + String(autoGals) + " gals </h2>");          
            if (autoModeStatetxt=="Off") {
              client.println("<p><a href=\"/auto\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/valveClose\"><button class=\"button button2\">OFF</button></a></p>");
            }
           client.println("<p><a href=\"/\"><button class=\"button button3\">Refresh</button></a></p>");
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

