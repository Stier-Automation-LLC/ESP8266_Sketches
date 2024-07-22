/* 
  ===========================================================================
  Dimple Garden Lighting System
  ---------------------------------------------------------------------------
  Enables Relay in Dimple Garden and Reads and Displays the temperature
  and Humudity using a DHT11 sensor. 
    The circuit:
      D1 = GPIO5  - Output - Relay
      D2 = GPIO4  - Input - On Pushbutton
      D3 = GPIO0  - Output  - GreenLED - WIFI Coneected
      D4 = GPIO2  - Output - InternalLED - blinks where program is running 

      D5 - GPIO14  - Input - off Pushbutton
      D6 - GPIO12 - Input - DHT11 Humidity sesor
  ---------------------------------------------------------------------------
  Revison		  Date		    Whom			  What
  0.0.1 	    7/18/2022   S. Stier	 	Initial Release


  ===========================================================================
*/
// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <DHT.h> //Including library for dht

const String version = "0.0.1";

//GPIO Assingnments
const int relayGPIO = 5; //Relay GPIO
const int wifiLEDGPIO = 0;  // YellowWifi LED GPIO
const int internalLEDGPIO = 2;  //Internal LED GPIO
const int startPBGPIO = 4;  // Start PB GPIO
const int stopPBGPIO = 14; // Stop PB GPIO


const int debug = LOW;
int watchDogLEDState = LOW;  // ledState used to set the LED

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
long previousHeartbeatMillis = 0; //holds the time the HeartBeat LED last blinked
const long heartBeatInterval = 2000;  // interval at which to blink Heartbeat LED (milliseconds)

String relayStatetxt;

// the media access control (ethernet hardware) address for the shield:
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xDF, 0x44 };  
// Only need to set this if you want to use static IPs
byte ip[] = { 192, 168, 100, 11 };   //Manual setup only
byte gateway[] = { 192, 168, 100, 1 }; //Manual setup only
byte subnet[] = { 255, 255, 255, 0 }; //Manual setup only

// Replace with your network credentials
const char* ssid     = "pugfi";
const char* password = "cupcake2016";

int count;

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

#define DHTPIN 12 //PIN WHERE DHT11 is connected D6=gpio12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  //start serial connection
  Serial.begin(9600);
  // relay
  pinMode(relayGPIO, OUTPUT);
  //LEDS
  pinMode(wifiLEDGPIO, OUTPUT);  
  pinMode(internalLEDGPIO, OUTPUT); 
  //set the pushbuttons to  enable the internal pull-up resistor
  pinMode(startPBGPIO, INPUT_PULLUP);
  pinMode(stopPBGPIO, INPUT_PULLUP);

  dht.begin();

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
  digitalWrite(wifiLEDGPIO, HIGH); 
}


void loop() {
  // Get the System clock
  unsigned long currentMillis = millis();
  float temp ;
  float humidity;

  // Flash the internal LED every 2 second as as a hearbeat signal
  if(currentMillis - previousHeartbeatMillis > heartBeatInterval) {
    previousHeartbeatMillis  = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (watchDogLEDState == LOW)
      watchDogLEDState = HIGH;
    else
      watchDogLEDState = LOW;
    
    digitalWrite(internalLEDGPIO, watchDogLEDState); 

    // Read temperature as Fahrenheit (isFahrenheit = true)
    temp = dht.readTemperature(true);
    // Read the Humidity
    humidity = dht.readHumidity();


    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp)) {
      Serial.println("Failed to read from DHT sensor!");
    }

  }

  //read the pushbutton value into a variable
  int startPBState = digitalRead(startPBGPIO);
  int stopPBState = digitalRead(stopPBGPIO);
  

  // Open the valve when the Open Pushbutton is pressed
  if (startPBState == LOW) {
    digitalWrite(relayGPIO, HIGH); 
    Serial.println("Relay On from PushButton.");
  } 
  // Close the valve when the Open Pushbutton is pressed
  if (stopPBState == LOW) {
    digitalWrite(relayGPIO, LOW); 
    Serial.println("Relay Off from PushButton.");
  } 

  int relayState = digitalRead(relayGPIO);

  if (relayState == LOW) {
    relayStatetxt = "Off";
  } else {
    relayStatetxt = "On";
  }
  

  if (debug == HIGH) {
    Serial.print("startPB=");
    Serial.print(startPBState);
    Serial.print(",stopPB=");
    Serial.print(stopPBState);
    Serial.print(",relay=");
    Serial.print(relayState);
    Serial.println("");
        //prints out the Temperature and Humidity to serial Chanel
    Serial.printf("Temp: %.1f°F Humidity: %.0f \n",temp,humidity);  

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
        //Serial.write(c);                    // print it out the serial monitor
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
          if (header.indexOf("GET /relayOn") >= 0) {
              Serial.println("Relay On from INET");
              digitalWrite(relayGPIO, HIGH);
              relayStatetxt = "On";
            } else if (header.indexOf("GET /relayOff") >= 0) {
              Serial.println("Relay off from INET");
              digitalWrite(relayGPIO, LOW); 
              relayStatetxt = "Off";
            } 
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            //client.println("<head><meta http-equiv=\"refresh\" content=\"5\">");
    
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}");
            client.println(".button3 {background-color: #008000;}</style>");
            client.println("</head>");

            // Web Page Heading
            client.println("<body><h1>Dimple Garden Lights</h1>");;


            client.println("<h2>Relay: " +relayStatetxt+ "</h2>");
            // If the output5State is off, it displays the ON button       
            if (relayStatetxt=="On") {
              client.println("<h2><a href=\"/relayOff\"><button class=\"button button2\">Off</button></a></h2>");
            } else {
              client.println("<h2><a href=\"/relayOn\"><button class=\"button\">On</button></a></h2><h2>");
            } 
            temp = dht.readTemperature(true);
            // Read the Humidity
            humidity = dht.readHumidity();

            client.printf("Temp: %.1f degF   Humidity: %.0f \n",temp,humidity);       
            // Display Auto Mode Status
            //client.println("<h2>Auto Mode " + autoModeStatetxt + "</h2>");
            client.println("</h2><h2><a href=\"/\"><button class=\"button button3\">Refresh</button></a></h2>");
            client.println("<p>Version:" + version + " Count:" + String(count++) + "</p>");
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

