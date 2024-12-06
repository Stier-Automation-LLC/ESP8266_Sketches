/*
  ===========================================================================
  Dimple Garden Light Switch
  ---------------------------------------------------------------------------
  Steven Stier Stier Automation LLC

  Dimple Garden Lights Switch, temp and humidity

  The circuit:
      D1 = GPIO5  - Output - Relay
      D2 = GPIO4  - Input - On Pushbutton
      D3 = GPIO0  - Output  - wifiLED - WIFI Coneected
      D4 = GPIO2  - Output - InternalLED - blinks where program is running 

      D5 - GPIO14  - Input - off Pushbutton
      D6 - GPIO12 -  Input - DHT11 Humidity sesor

  ---------------------------------------------------------------------------
  Revison		  Date		      Whom			  What
  0.0.1 	    7/30/2024     S. Stier	 	Initial Release
  0.0.2 	    8/10/2024     S. Stier	 	install to garden
  0.0.3 	    12/6/2024    S. Stier	 	Update IP address and MQTT TOPIC Prefixes
  ===========================================================================
*/

#include <ESP8266WiFi.h> // library specifically for the ESP8266 WiFi
#include <DHT.h> //Including library for dht sensor
#include <Ticker.h> //esp8266 library that calls functions periodically
#include <AsyncMqttClient.h>

const String version = "0.0.3";
const int debug = HIGH;
const String webSiteHeader = "Dimple ESP Test";

/* Set your Static IP address info
192.168.100.42	pugyardwater.puglocal	pugNetwork Yard Water System
192.168.100.43	pugswitch.puglocal	pugNetwork Garden Light Switch
192.168.100.44	puggwater.puglocal	pugNetwork Garden Water System
192.168.100.45	pugTemp.puglocal	Pugnetwork Temp Monitoring System
192.168.100.46	pugESPTest.puglocal	Pugnetwork ESP Test System
*/
IPAddress local_IP(192, 168, 100, 46);
IPAddress gateway(192, 168, 100, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 100, 1);  
IPAddress secondaryDNS(8, 8, 8, 8); 

// WIFI Settings
#define WIFI_SSID "pugfi"
#define WIFI_PASSWORD "cupcake2016"

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

int screencounter;

//  MQTT Mosquitto Broker Settings
//#define MQTT_HOST IPAddress(192, 168, 100, 40)

#define MQTT_HOST "pugsvr.puglocal"
#define MQTT_PORT 1883

#define MQTT_USERNAME "mqadmin"
#define MQTT_PASSWORD "fred"

// MQTT Topic Names
/* Set your Static IP address info
const String  MQTT_TOPIC_PREFIX ="pugland/yard/water/"; // pugNetwork Yard Water System
const String  MQTT_TOPIC_PREFIX ="pugland/garden/switch/"; // pugNetwork Garden Light Switch
const String  MQTT_TOPIC_PREFIX ="pugland/garden/water/"; // pugNetwork Garden Water System
const String  MQTT_TOPIC_PREFIX ="pugland/house/temp/"; // 	Pugnetwork Temp Monitoring System
const String  MQTT_TOPIC_PREFIX ="pugland/test/switch/"; // 	Pugnetwork ESP Test System
*/
const String  MQTT_TOPIC_PREFIX ="pugland/test/switch/"; // Must be unique for all devices
const String  MQTT_TOPIC_HB = "hb";
const String  MQTT_TOPIC_TEMPERATURE = "temperature";
const String  MQTT_TOPIC_HUMIDITY = "humidity";
const String  MQTT_TOPIC_RELAY = "relay";

//GPIO Assignments
#define RELAYGPIO 5 //GPIO WHERE LED Connected D1=gpio5
#define STARTPBGPIO 4 //GPIO Where Start PB connected D2=GPIO4
#define WIFILEDGPIO 0 //GPIO where WIFI LED Connected  D3=gpio0
#define INTERNALLEDGPIO 2 //GPIO WHERE LED Connected D4=gpio2
#define STOPPBGPIO 14  //GPIO where Stop PB Connected D5 =GPIO14
#define DHTGPIO 12 //GPIO WHERE DHT11 is connected D6=gpio12

#define DHTTYPE DHT11
DHT dht(DHTGPIO, DHTTYPE);

Ticker wifiReconnectTimer; // timer to reconnect to wifi

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer; // time to reconnect to MQTT

String temperature = "Bad"; // Tempurature Sensor Reading.
String humidity = "Bad";  // Humidity Sensor Reading.

String prevTemperature; // Previous Good Temperature Sensor Reading.
String prevHumidity;   // Previous Good Humidity Sendor Reading.

bool internalLEDState;
String relayState;
String prevRelayState;

unsigned long prevMillisOneSecond = 0; // Stores Last time One Second function called

unsigned long prevMillisSensorRead = 0;   // Stores last time Sensor Read Function Called

const long intervalSensorRead = 10000;         // Interval at which to read MQTTUpdateAll
unsigned long sensorCount = 0;

unsigned long prevMillisMQTTUpdateAll = 0;   // Stores last time MQTTUpdateAll was published
const long intervalMQTTUpdateAll = 60000;         // Interval at which to publish sensor readings


// Current time
unsigned long currentETime = millis();
// Previous time
unsigned long previousETime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutETime = 2000;

String relayStatetxt;

int i = 0;


void connectToWifi() {
  Serial.printf("Connecting to Wi-Fi... -- WIFI_SSID: %s  ,WIFI_PASSWORD: %s \n", WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.printf("Success:) Connected to Wi-Fi. -- WIFI_SSID: %s  ,WIFI_PASSWORD: %s ,IP Address: ", WIFI_SSID, WIFI_PASSWORD);
  Serial.println(WiFi.localIP());
  connectToMqtt();
  server.begin();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("WARNING! Disconnected from Wi-Fi. Trying again in 10 Seconds.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  digitalWrite(WIFILEDGPIO, LOW);
  wifiReconnectTimer.once(10, connectToWifi);
}

void connectToMqtt() {
  Serial.printf("Connecting to MQTT Broker.... -- MQTT_HOST: %s ,MQTT_USERNAME: %s ,MQTT_PASSWORD: %s \n", MQTT_HOST, MQTT_USERNAME, MQTT_PASSWORD);
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.printf("Success:) Connected to MQTT Broker.  -- MQTT_HOST: %s ,MQTT_USERNAME: %s ,MQTT_PASSWORD: %s , SessionPresent: %i \n", MQTT_HOST, MQTT_USERNAME, MQTT_PASSWORD, sessionPresent);
    Serial.println("Connected to MQTT.");
  
  // subscribe to relay CMD topic
  String relayCMDTopic = String(MQTT_TOPIC_PREFIX + MQTT_TOPIC_RELAY + "CMD");
  
  uint16_t packetIdSub = mqttClient.subscribe(relayCMDTopic.c_str(), 0);
  Serial.println("Subscribing to relay Command at QoS 0");
  
  digitalWrite(WIFILEDGPIO, HIGH);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.printf("WARNING! Disconnected from MQTT Broker. HOST: %s   \n", MQTT_HOST);

  if (WiFi.isConnected()) {
    Serial.println("Trying MQTT connect again in 10 Seconds...");
    mqttReconnectTimer.once(10, connectToMqtt);
  }
  digitalWrite(WIFILEDGPIO, LOW);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  if (debug == HIGH) {
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
  }
}

void onMqttUnsubscribe(uint16_t packetId) {
  if (debug == HIGH) {
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

// You can modify this function to handle what happens when you receive a certain message in a specific topic
void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String messageTemp;
  for (int i = 0; i < len; i++) {
    //Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  String relayCMDTopic = String(MQTT_TOPIC_PREFIX + MQTT_TOPIC_RELAY + "CMD");
  Serial.print("relayCMDTopic=");
  Serial.print(relayCMDTopic);
  // Check if the MQTT message was received on topic test
  if (strcmp(topic, relayCMDTopic.c_str()) == 0) {
    Serial.println("Valve CMD Topic Recieved");
    if (relayCMDTopic == "1") {
      // Engage the relay
      digitalWrite(RELAYGPIO, HIGH); 
      Serial.println("Relay On from MQTT.");
    } else {
    // Disengage the relay
      digitalWrite(RELAYGPIO, LOW); 
      Serial.println("Relay Off from MQTT.");
    }
  }
  // Serial.println("Publish received.");
  // Serial.print("  message: ");
  // Serial.println(messageTemp);
  // Serial.print("  topic: ");
  // Serial.println(topic);
  // Serial.print("  qos: ");
  // Serial.println(properties.qos);
  // Serial.print("  dup: ");
  // Serial.println(properties.dup);
  // Serial.print("  retain: ");
  // Serial.println(properties.retain);
  // Serial.print("  len: ");
  // Serial.println(len);
  // Serial.print("  index: ");
  // Serial.println(index);
  // Serial.print("  total: ");
  // Serial.println(total);
}


void onMqttPublish(uint16_t packetId) {
  // if (debug == HIGH) {
  //   Serial.print("Publish acknowledged.");
  //   Serial.print("  packetId: ");
  //   Serial.println(packetId);
  // }
}

void sensorRead() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float t = dht.readTemperature(true);
  // Read the Humidity
  float h = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Warning! : Failed to read from DHT sensor.");
    temperature = "BAD";
    humidity = "BAD";
    return;
  }
  // Prints out the Temperature and Humidity to serial Chanel
  if (debug == HIGH) {
    Serial.printf("%i Temp: %.1f°F Humidity: %.0f \n",sensorCount++,t,h);  
  }

  // format the temperature
  temperature = String(t,1);
  // determine if the value changed
  if (temperature != prevTemperature) {
    sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_TEMPERATURE,temperature);
  }
  prevTemperature = temperature;
  
  // format the humidity
  humidity = String(h,0);
  if (humidity != prevHumidity) {
    sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_HUMIDITY,String(humidity));
  }
  prevHumidity = humidity;

}

void OneSecondFunction() {
  //turn the Intrnal LED on and off
  internalLEDState = !internalLEDState;
  digitalWrite(INTERNALLEDGPIO, internalLEDState);
}

void updateAllMQTT() {
  // update all data
  if (temperature != "BAD"){
    sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_TEMPERATURE,temperature);
  }
  if (humidity != "BAD"){
    sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_HUMIDITY,humidity);
  }

  sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_RELAY,relayState);  

  // heartbeat topic
  String hbString = "hb " + String(i);
  sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_HB,hbString);
  i++;
}

void sendMQTTUpdate(String topic, String value ) {
    // Publish an MQTT message for the topic and value
    uint16_t packetIdPub1 = mqttClient.publish(String(topic).c_str(), 1, true, String(value).c_str());                            
    Serial.printf("Publishing on topic: %s at QoS 1, packetId: %i", String(topic).c_str(), packetIdPub1);
    Serial.printf(" value: %s \n", String(value));;
}

void setup() {
  Serial.begin(9600);
  Serial.println();

  
  pinMode(RELAYGPIO, OUTPUT);
  pinMode(INTERNALLEDGPIO, OUTPUT);
  pinMode(WIFILEDGPIO, OUTPUT);
  pinMode(STARTPBGPIO, INPUT_PULLUP);
  pinMode(STOPPBGPIO, INPUT_PULLUP);

  dht.begin();

  internalLEDState = false;

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(MQTT_USERNAME, MQTT_PASSWORD);
  
  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("WIFI Failed to configure");
  }
  connectToWifi();
}

void loop() {

  //read the pushbutton values into a variable
  int startPBState = digitalRead(STARTPBGPIO);
  int stopPBState = digitalRead(STOPPBGPIO);
  
  // Energize the Relay when the start PB is pressed
  if (startPBState == LOW) {
    digitalWrite(RELAYGPIO, HIGH); 
    Serial.println("Relay On from PushButton.");
  } 

  // Deenergize the Relay when the stop PB is pressed
  if (stopPBState == LOW) {
    digitalWrite(RELAYGPIO, LOW); 
    Serial.println("Relay Off from PushButton.");
  } 


  // Get the value of the relay
  relayState = String(digitalRead(RELAYGPIO));

  if (relayState != prevRelayState) {
    sendMQTTUpdate(MQTT_TOPIC_PREFIX + MQTT_TOPIC_RELAY,relayState);  
  }
  prevRelayState = relayState;
  
 if (relayState == "0") {
    relayStatetxt = "Off";
  } else {
    relayStatetxt = "On";
  }

  unsigned long currentMillis = millis();

  // Call the one second Function every second.
   if (currentMillis - prevMillisOneSecond >= 1000) {
    prevMillisOneSecond = currentMillis;
    OneSecondFunction();
  }

  // Every X number of seconds read the sensors (interval = 10 secs) 
   if (currentMillis - prevMillisSensorRead >= intervalSensorRead) {
    // Save the last time a new reading was published
    prevMillisSensorRead = currentMillis;
    sensorRead();
  }

  // force update of all MQTT varibles at a particular interval
  if (currentMillis - prevMillisMQTTUpdateAll >= intervalMQTTUpdateAll) {
    // Save the last time a new reading was published
    prevMillisMQTTUpdateAll = currentMillis;
    updateAllMQTT();
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
              digitalWrite(RELAYGPIO, HIGH);
              relayStatetxt = "On";
            } else if (header.indexOf("GET /relayOff") >= 0) {
              Serial.println("Relay off from INET");
              digitalWrite(RELAYGPIO, LOW); 
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
            client.println("<body><h1>" + webSiteHeader + "</h1>");;


            client.println("<h2>Relay: " +relayStatetxt+ "</h2>");
            // If the output5State is off, it displays the ON button       
            if (relayStatetxt=="On") {
              client.println("<h2><a href=\"/relayOff\"><button class=\"button button2\">Turn Off</button></a></h2>");
            } else {
              client.println("<h2><a href=\"/relayOn\"><button class=\"button\">Turn On</button></a></h2><h2>");
            } 

            //client.printf("Temp: %.1f degF   Humidity: %.0f \n",temperature,humidity);  
            client.println("<h2>Temperature:" + temperature + " degF Humidity:" + humidity + "</h2>");     
            // Display Auto Mode Status
            //client.println("<h2>Auto Mode " + autoModeStatetxt + "</h2>");
            client.println("<h2><a href=\"/\"><button class=\"button button3\">Refresh</button></a></h2>");
            client.println("<p>Version:" + version + " Count:" + String(screencounter++) + "</p>");
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