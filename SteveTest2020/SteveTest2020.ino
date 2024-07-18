/*
 Steves Watering system
 V0.0.2
 */

#include <Ethernet.h>
#include <SPI.h>
#include <string.h>
#include<stdlib.h>

boolean reading = false;

// the media access control (ethernet hardware) address for the shield:
   byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0xDF, 0x44 };  
// Only need to set this if you want to use static IPs
   byte ip[] = { 192, 168, 100, 11 };   //Manual setup only
   byte gateway[] = { 192, 168, 100, 1 }; //Manual setup only
   byte subnet[] = { 255, 255, 255, 0 }; //Manual setup only

// PIN Assignment
  
    const int HeartbeatLEDpin =  4;  // the number of the pin for the LED (White) that Blinks when program runs
    const int  ValvePin1 = 5;    // the pin that Valve 1 is Set to
    const int  ValvePin2 = 6;    // the pin that Valve 2 is Set to
    const int  ValvePin3 = 7;    // the pin that Valve 3 is Set to
    const int  ValvePin4 = 8;    // the pin that Valve 4 is Set to
    const int  ValvePin5 = 9;    // the pin that Valve 5 is Set to
    
    const int RedLEDpin =  30;  // the number of the pin for the RED LED
    const int RedLEDpin1 =  31;  // the number of the pin for the Alternate RED LED
    const int GreenLEDpin =  32;  // the number of the pin for the Green LED
    const int GreenLEDpin1 =  33;  // the number of the pin for the Alternate Green LED
     
    const int RedButtonpin =  22;  // the number of the Pin for the Red PushButton
    const int switchpin =  24;  // the number of the Pin for the ON/OFF Switch
    const int BlackButtonpin =  26;  // the number of the Pin for the Black Push Button
     
    const int Flowpin1 =  36;  // the number Pin for Flow Switch 1
    const int Flowpin2 =  37;  // the number Pin for Flow Switch 2
    const int Flowpin3 =  38;  // the number Pin for Flow Switch 3
    const int Flowpin4 =  39;  // the number Pin for Flow Switch 4
    const int Flowpin5 =  40;  // the number Pin for Flow Switch 5
    const int FlowMeterpin =  3;  // the number Pin for Flow Switch 6
    
    // these hold the current values of the PINS
    
    int RedPushButton = LOW;
    int SwitchState = LOW;
    int BlackPushButton = LOW;
    int Flow1 = LOW;
    int Flow2 = LOW;
    int Flow3 = LOW;
    int Flow4 = LOW;
    int Flow5 = LOW;
    int FlowMeter = LOW;
    
    int Valve1 = LOW;
    int Valve2 = LOW;
    int Valve3 = LOW;
    int Valve4 = LOW;
    int Valve5 = LOW; 
    
    long previousHeartbeatMillis = 0;
    long previousMillis = 0;        // will store last time LED was updated
    int ledState = LOW;             // ledState used for the heartbeat LED
    long interval = 10000;           // interval at which to blink (milliseconds)
     

   unsigned long windowCount=0;
   unsigned long ClicksPer10secs=0;
   
   //counters for each flowmeter
   unsigned long TotalClicks = 0;
   //latches so the counter doesn't get stuck counting if the magnet stays next to the sensor
   boolean SensorIsLowThisScan = false;
   boolean SensorWasHighLastScan= false;

   unsigned long totalizer=0;
   float gals=0;
   char test[20];
    
   EthernetServer server = EthernetServer(80);

 
void setup() {
  // initialize the Pins
    // Valve Outputs
    pinMode(ValvePin1, OUTPUT);
    pinMode(ValvePin2, OUTPUT);
    pinMode(ValvePin3, OUTPUT);
    pinMode(ValvePin4, OUTPUT);
    pinMode(ValvePin5, OUTPUT);
    
    pinMode(HeartbeatLEDpin, OUTPUT); 
    pinMode(RedLEDpin, OUTPUT);
    pinMode(RedLEDpin1, OUTPUT);
    pinMode(GreenLEDpin, OUTPUT);
    pinMode(GreenLEDpin1, OUTPUT);
 
    pinMode(BlackButtonpin, INPUT);
    pinMode(RedButtonpin, INPUT);
    pinMode(switchpin, INPUT);
 
    pinMode(Flowpin1, INPUT);
    pinMode(Flowpin2, INPUT);
    pinMode(Flowpin3, INPUT);
    pinMode(Flowpin4, INPUT);
    pinMode(Flowpin5, INPUT);
    
    pinMode(FlowMeterpin, INPUT);   
    
    attachInterrupt(1, IncrementFlow, RISING); // Note: 0 (Pin2), 1 (Pin3), 2 (pin 21), 3 (pin 20), 4 (pin 19), and 5 (pin 18).

      
   
   // initialize serial communication:
   //Serial.begin(9600);
   
  //Ethernet.begin(mac); // if using DHCP
  Ethernet.begin(mac, ip, gateway, subnet); //for manual setup

  server.begin();
  //Serial.println(Ethernet.localIP());
}

 void loop() {
  
    BlackPushButton  = digitalRead(BlackButtonpin);
    RedPushButton = digitalRead(RedButtonpin);
    SwitchState = digitalRead(switchpin);
    Flow1 = digitalRead(Flowpin1);
    Flow2 = digitalRead(Flowpin2);
    Flow3 = digitalRead(Flowpin3);
    Flow4 = digitalRead(Flowpin4);
    Flow5 = digitalRead(Flowpin5);
    FlowMeter = digitalRead(FlowMeterpin);
   
if (gals > 4)  {
//closes all the values as a safety
digitalWrite(ValvePin1, LOW);
digitalWrite(ValvePin2, LOW);
digitalWrite(ValvePin3, LOW);
digitalWrite(ValvePin4, LOW);
digitalWrite(ValvePin5, LOW);
totalizer = 0;
gals = 0;  
}


   
   unsigned long currentMillis = millis();
  
    if(currentMillis - previousHeartbeatMillis > 1000) {
         // save the last time you blinked the Heartbeat LED 
        previousHeartbeatMillis  = currentMillis;   
    
        // if the LED is off turn it on and vice-versa:
         if (ledState == LOW)
           ledState = HIGH;
         else
           ledState = LOW;
     
        // set the LED with the ledState of the variable:
         digitalWrite(HeartbeatLEDpin, ledState);   
     }
  
   if(currentMillis - previousMillis > interval) {
       previousMillis = currentMillis;   
       ClicksPer10secs = windowCount;
       windowCount = 0; 
   }
   // listen for incoming clients, and process qequest.
  checkForClient();
}

void IncrementFlow() {
    windowCount++; //increment window counter (counts/unit time)
    TotalClicks++; //cumulative count for flowmeter
    totalizer++; //cumulative count for totalizer
    gals = totalizer/1600;
}

void checkForClient(){

  EthernetClient client = server.available();

  if (client) {

    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;

    while (client.connected()) {
      if (client.available()) {

        if(!sentHeader){
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: application/json");
          client.println("Access-Control-Allow-Origin: *");
          client.println("Access-Control-Allow-Methods: GET");
          
          client.println();
          sentHeader = true;
        }

        char c = client.read();

        if(reading && c == ' ') reading = false;
        if(c == '?') reading = true; //found the ?, begin reading the info

        if(reading){
          Serial.print(c);

           switch (c) {
            case '1':
              //Open Valve 1
              ResetTotalizer();
              CloseAllValves(); 
              digitalWrite(ValvePin1, HIGH);
              break;
            case '2':
            //Open Valve 2
              ResetTotalizer();
              CloseAllValves(); 
              digitalWrite(ValvePin2, HIGH);
              break;
            case '3':
            //Open Valve 3
              ResetTotalizer();
              CloseAllValves(); 
              digitalWrite(ValvePin3, HIGH);
              break;
            case '4':
            //Open Valve 4
              ResetTotalizer();
              CloseAllValves(); 
              digitalWrite(ValvePin4, HIGH);
              break;
            case '5':
            //Open Valve 5
              ResetTotalizer();
              CloseAllValves(); 
              digitalWrite(ValvePin5, HIGH);
              break;
           // Send A,B,C,D,E to close the appropriate Value
           case 'A':
            //Close Valve 1
              digitalWrite(ValvePin1, LOW);
              break;
            case 'B':
             //Close Valve 2
              digitalWrite(ValvePin2, LOW);
              break;
            case 'C':
             //Close Valve 3
              digitalWrite(ValvePin3, LOW);
              break;
            case 'D':
             //Close Valve 4
              digitalWrite(ValvePin4, LOW);
              break;
            case 'E':
             //Close Valve 5
              digitalWrite(ValvePin5, LOW);
              break;   
             case 'F':
             //Close all Valves
             CloseAllValves(); 
          }

        }

        if (c == '\n' && currentLineIsBlank)  break;

        if (c == '\n') {
          currentLineIsBlank = true;
        }else if (c != '\r') {
          currentLineIsBlank = false;
        }

      }
    }
    //format the JSON object returned
    client.print("{");
    formatJSONLine("valve1",digitalRead(ValvePin1),client);
    formatJSONLine("valve2",digitalRead(ValvePin2),client);
    formatJSONLine("valve3",digitalRead(ValvePin3),client);
    formatJSONLine("valve4",digitalRead(ValvePin4),client);
    formatJSONLine("valve5",digitalRead(ValvePin5),client);
    
    formatJSONLine("blackpb",digitalRead(BlackButtonpin),client);
    formatJSONLine("redpb",digitalRead(RedButtonpin),client);
    formatJSONLine("switch",digitalRead(switchpin),client);
    formatJSONLine("flow1",digitalRead(Flowpin1),client);
    formatJSONLine("flow2",digitalRead(Flowpin2),client);
    formatJSONLine("flow3",digitalRead(Flowpin3),client);
    formatJSONLine("flow4",digitalRead(Flowpin4),client);
    formatJSONLine("flow5",digitalRead(Flowpin5),client);
    
    formatJSONLine("totalclicks",TotalClicks,client);
    formatJSONLine("clickper10",ClicksPer10secs,client);
    formatJSONLine("totalizer",totalizer,client);
    client.print("\"gals\":");
    client.println(dtostrf(gals,0,2,test));
    client.println("}");
    client.println("");
    delay(1); // give the web browser time to receive the data
    client.stop(); // close the connection:
  } 

}
void formatJSONLine(String name, int Value, EthernetClient client) {
  client.println("\""+ name+"\""+":"+ Value + ",");
}

void ResetTotalizer () {
  totalizer = 0;
}

void CloseAllValves () {
digitalWrite(ValvePin1, LOW);
digitalWrite(ValvePin2, LOW);
digitalWrite(ValvePin3, LOW);
digitalWrite(ValvePin4, LOW);
digitalWrite(ValvePin5, LOW);
}
