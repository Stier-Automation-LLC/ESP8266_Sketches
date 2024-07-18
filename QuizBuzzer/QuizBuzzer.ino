/*
  Quiz buzzer
  12/31/2022
  Steven Stier Stier Automation LLC

  Determines which team hits thier pushbutton first and lights the respective light. Reset pushbuttone resets choice. 
  Buzzer sounds when either team gets choice.

  The circuit:
  - Ped Team LEDs attached from pin 2  to ground through 220 ohm resistor
  - Green Team LEDs attached from pin 3  to ground through 220 ohm resistor
  - Buzzer attached from pin 8 to ground through 220 ohm resistor

  - Red Team pushbutton attached to pin 9 from +5V
  - 10K resistor attached to pin 9 from ground

  - Green Team pushbutton attached to pin 10 from +5V
  - 10K resistor attached to pin 10 from ground

  - Reset pushbutton attached to pin 7 from +5V
  - 10K resistor attached to pin 7 from ground

*/

// constants won't change. They're used here to set pin numbers:
const int redTeamLed= 2;  // the number of the Red Teams LED
const int greenTeamLed= 3;  // the number of the Green Teams LED

const int redTeamPB = 9;  // the number of the Red Teams Pushbutton
const int greenTeamPB = 10;  // the number of the Green Teams Pushbutton

const int resetPB = 7;    // the number of the reset Pushbutton
const int buzzerPin = 8;    // the number of the pin attached to Buzzer

// variables will change:
int redPBState = 0;  
int greenPBState = 0;
int resetPBState = 0;
int RedTeamFirst = 0;
int GreenTeamFirst = 0;
int buzzerEnabled = 0;


void setup() {
  
  // Initize the PINS
  pinMode(redTeamLed, OUTPUT); 
  pinMode(greenTeamLed, OUTPUT);  
  pinMode(buzzerPin, OUTPUT);  
  pinMode(redTeamPB, INPUT);
  pinMode(greenTeamPB, INPUT);
  pinMode(resetPB, INPUT);
}

void loop() {
  // read the state of the pushbuttons
  redPBState = digitalRead(redTeamPB);  
  greenPBState = digitalRead(greenTeamPB);
  resetPBState =digitalRead(resetPB);
  
  //Determine who first hit thier pushbutton
  if ((redPBState == HIGH)  && (GreenTeamFirst == 0)) {
    RedTeamFirst = 1;
  }
  if ((greenPBState == HIGH)  && (RedTeamFirst == 0)) {
    GreenTeamFirst = 1;
  }
  if (resetPBState== HIGH) {
    RedTeamFirst = 0;
    GreenTeamFirst = 0;
    buzzerEnabled = 0;  
  }

  // LIght up the correspong pushbutton
  if (RedTeamFirst == 1) {
    digitalWrite(redTeamLed, HIGH);
  } else {
    digitalWrite(redTeamLed, LOW);
  }

  if (GreenTeamFirst == 1) {
    digitalWrite(greenTeamLed, HIGH);
  } else {
    // turn LED off:
    digitalWrite(greenTeamLed, LOW);
  }

  //run the buzzer for 1 second after either team is first.
  if (((GreenTeamFirst == 1) || (RedTeamFirst == 1)) && (buzzerEnabled == 0)) {
  buzzerEnabled = 1;
  digitalWrite(buzzerPin, HIGH);
  delay(1000); 
  digitalWrite(buzzerPin, LOW); 
  }

}

