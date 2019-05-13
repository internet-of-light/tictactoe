/*************************************************************
ESP8266 Tic Tac Toe
Philips Hue IOL DRG Spring 2019
*************************************************************/
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>


// Wifi network SSID
const char* ssid = "University of Washington";
// Wifi network password
const char* password = "";

// IP of Hue gateway
String ip = "172.28.219.179";
String api_token = "rARKEpLebwXuW01cNVvQbnDEkd2bd56Nj-hpTETB";


//0 = unclaimed
//1 = player 1
//2 = player 2
int tictactoeStates[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};



int buttonPins[] = {4, 16, 14, 5, 0, 12, 2, 15, 13};
int buttonToggles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; //For debouncing
//int b1 = 4;
//int b2 = 16;
//int b3 = 14;
//int b4 = 5;
//int b5 = 0;
//int b6 = 12;
//int b7 = 2;
//int b8 = 15;
//int b9 = 13;


#define DEBUG true
/*
   Debugging Print Functions
   Easier than putting if(DEBUG) in front of every print statement
*/
void dbprint(String in) {
  if (DEBUG) Serial.print(in);
}
void dbprintln(String in) {
  if (DEBUG) Serial.println(in);
}


//Push parameters to group
void changeGroup(byte groupNum, byte transitiontime, String parameter, String newValue, String parameter2 = "",
                 String newValue2 = "", String parameter3 = "", String newValue3 = "",
                 String parameter4 = "", String newValue4 = "");


//Push parameters to individual light
void changeLight(byte lightNum, byte transitiontime, String parameter, String newValue, String parameter2 = "",
                 String newValue2 = "", String parameter3 = "", String newValue3 = "",
                 String parameter4 = "", String newValue4 = "");


/*  checkLightStatus
    returns true or false (1 or 0)
    checks if light is on or off
    parameter: # of light
*/
bool checkLightStatus(byte lightNum);


/*  toggleLight
    Simply toggle the on/off status of a light
    transitiontime must be specified
    parameter: # of light
*/
void toggleLight(byte lightNum, byte transitiontime) {
  bool newStatus = !checkLightStatus(lightNum);
  changeLight(lightNum, transitiontime, "on", newStatus ? "true" : "false");
}

int turnNum = 0;


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  for (int pin : buttonPins) {
    if (pin == 15 or pin == 16) pinMode(pin, INPUT);
    else pinMode(pin, INPUT_PULLUP);
  }
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  dbprintln("Connected to the WiFi network");

}

unsigned long currentTime, buttonLastPressTime;

void loop() {
  currentTime = millis();
  buttonTrigger();
}


//Sieg Lower Floor
int lightIndexes[] = {10, 23, 11, 15, 7, 14, 22, 21, 16};

//--------------------- CHECKS WHICH BUTTON IS PRESSED THEN CHANGES THE STATE OF THE CORRESPONDING LIGHT ---------------------

void buttonTrigger() {
  for (int i = 0; i < 9; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (buttonPins[i] == 15) reading = !reading;
    //dbprintln("Button: " + String(i) + ", reading:" + String(reading));

    //If not pushed, reset toggle
    if (reading == HIGH) {
      buttonToggles[i] = 0;
    }
    if (reading == LOW and buttonToggles[i] == 0 and currentTime - buttonLastPressTime > 400) { //400ms pause, using buttonToggles for debouncing
      dbprintln("Button " + String(i) + " pressed.");
      toggleLight(lightIndexes[i], 2); //Toggle light state with 0.2s transitiontime
      buttonToggles[i] = 1;
      buttonLastPressTime = currentTime;
    }
  }
}




void turnManager(int lightBulb) {
  if (turnNum == 0) {

  }
  if (turnNum == 1) {

  }
}
