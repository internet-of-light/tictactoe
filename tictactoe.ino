/*************************************************************
  ESP8266 Tic Tac Toe
  Philips Hue IOL DRG Spring 2019

  Player 1 = BLUE, hue = 44000
  Player 2 = GREEN, hue = 2400
  Unclaimed color: "bri", "100", "hue", "9000" - somewhat warm white, dim(bri=100)

  The game resets itself if no moves have been made for 30 seconds
*************************************************************/
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h> //mqtt

#define TEXT_TESTING_MODE false //Disable lights and wifi, play tic tac toe in serial monitor
#define DEBUG true //View button presses and Hue API calls in serial monitor

//MQTT
const char* mqtt_server = "test.mosquitto.org";
const char* mqtt_username = "";
const char* mqtt_password = "";
const int mqtt_port = 1883;
#define DEVICE_MQTT_NAME "tictactoeHCDEIOL"


WiFiClient espClient;
PubSubClient client(mqtt_server, mqtt_port, espClient);

const char* ssid = "University of Washington"; // Wifi network SSID
const char* password = ""; // Wifi network password

bool GAME_RUNNING = false;
String ip = "172.28.219.179"; // Sieg Master IP
String api_token = "rARKEpLebwXuW01cNVvQbnDEkd2bd56Nj-hpTETB"; // Sieg Master API Token

//Sieg Lower Floor
int buttonPins[] = {14, 12, 13, 15, 0, 16, 2, 5, 4};
int lightIndexes[] = {10, 23, 11, 15, 7, 14, 22, 21, 16}; //Indexes of lights in Philips Hue system

//All possible combinations of 3 lights that result in a victory
int victoryCombinations[][3] = {{10, 15, 22}, {10, 7, 16}, {10, 23, 11}, {23, 7, 21}, {11, 7, 22}, {11, 14, 16}, {15, 7, 14}, {16, 21, 22}};

//0 = unclaimed
//1 = player 1
//2 = player 2
int tictactoeStates[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

int playerTakingTurn = 1; //always 1 or 2

int buttonToggles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; //For debouncing

unsigned long currentTime, buttonLastPressTime, lightLastUpdate; //timing

/*  setup
    Initialie button pins and serial communication and connect to WiFi
*/
void setup() {
  Serial.begin(115200);

  for (int pin : buttonPins) {
    if (pin == 0 or pin == 16 or pin == 14 or pin == 13 or pin == 12) pinMode(pin, INPUT);
    else pinMode(pin, INPUT_PULLUP);
  }

  if (!TEXT_TESTING_MODE) {
    setup_wifi();
  }
  buttonLastPressTime = 0;
  drawLights();
}


/*  loop
    check buttons and run tic tac toe game
*/
void loop() {


  if (!TEXT_TESTING_MODE) {
    //Connect to MQTT if not connected
    if (!client.connected()) {
      reconnect();
    }
    //Reconnect to WiFi if disconnected
    if (WiFi.status() != WL_CONNECTED) {
      delay(1);
      Serial.print("WIFI Disconnected. Attempting reconnection.");
      setup_wifi();
      return; //End this loop cycle if WiFi disconnected
    }
  }


  currentTime = millis(); //Update time (for debouncing)

  unsigned long timeSinceLastInput = currentTime - buttonLastPressTime;
  //Warning after 15 seconds
  if (timeSinceLastInput > 15000 and timeSinceLastInput < 15050) {
    dbprintln("15 seconds since last input");
  }

  //Reset after 30 seconds of no input
  if (timeSinceLastInput > 30000) {
    dbprintln("Timeout - resetting game");
    buttonLastPressTime = currentTime;
    resetGame();

  }

  checkButtons(); //Check buttons and trigger actions

  if (currentTime - lightLastUpdate > 2000) { //Update lights once per second
    if (TEXT_TESTING_MODE) {
      printGameState(); //Instead of pushing to lights, view game status in serial monitor
    } else {
      //drawLights(); //push game information to Philips Hue lights
    }
    lightLastUpdate = currentTime;
  }


  //Check if a player has won
  //Planning to implement: if player wins, whole screen goes to their color
  //for 5(?) seconds before resetting
  checkForVictory();


  //If every space is claimed and no one won (locked up) reset game
  checkForFullBoard();
}


/* checkButtons
   checks all of the buttons to see if they have been pressed
   If one has and anti-debouncing measures are satisfied,
   the corresponding light is switched if it a valid move
*/
void checkButtons() {
  for (int i = 0; i < 9; i++) {
    int reading = digitalRead(buttonPins[i]);
    if (buttonPins[i] == 15 or buttonPins[i] == 16) reading = !reading; //Invert reading for pins using external resistor

    //View states of all buttons - for testing default states
    //dbprintln("Button: " + String(i) + ", reading:" + String(reading));

    //If not pushed, reset toggle - for debouncing
    if (reading == HIGH) {
      buttonToggles[i] = 0;
    }

    //If it is pushed,
    //500ms pause, using buttonToggles for debouncing
    if (reading == LOW and buttonToggles[i] == 0 and currentTime - buttonLastPressTime > 500) {
  
      //MQTT - SEND STATE 1 (ON)
      sendState(1);
      GAME_RUNNING = true;

      dbprintln("Button " + String(i) + " pressed.");
      tttManager(lightIndexes[i]);
      drawLights();
      buttonToggles[i] = 1; //set buttonToggle to 1 so that we know that this button isn't being held down
      buttonLastPressTime = currentTime; //Stop player from accidentally taking next players turn by hitting 2 buttons fast
    }
  }
}





/* ttt (tic tac toe) Manager
   Called when a button is pressed
   Updates the state of the corresponding
   light if it was a valid move
*/
void tttManager(int lightNum) {
  int lightOwner = ownsLight(lightNum);
  int lightIndex = lightNumToIndex(lightNum);
  if (playerTakingTurn == 1) { //If player 1 pressed the button
    if (lightOwner == 0) { //if the light is unclaimed
      tictactoeStates[lightIndex] = 1; //switch light to player 1
      playerTakingTurn = 2; //switch to player 2's turn
    }
    //If player 1 or 2 already owns the light, we don't need to do anything
    //Could provide some indication that it was not a valid move
    if (lightOwner == 1 or lightOwner == 2) {
      //Flash led strip red
    }
  }
  else if (playerTakingTurn == 2) {//If player 2 pressed the button
    if (lightOwner == 0) { //if the light is unclaimed
      tictactoeStates[lightIndex] = 2; //switch light to player 2
      playerTakingTurn = 1; //switch to player 1's turn
    }
    //If player 1 or 2 already owns the light, we don't need to do anything
    //Could provide some indication that it was not a valid move
    if (lightOwner == 1 or lightOwner == 2) {

    }
  }
}

/* drawLights
   Push the appropriate colors to the lights
   depending on their status in the tic tac toe game
   Should not be called every draw cycle - needs a timer

   Sending commands to Philips Hue Bridge too fast
   results in "Internal 404" errors from the Bridge
*/
void drawLights() {
  for (int i = 0; i < 9; i++) {
    int lightNum = lightIndexes[i];
    switch (tictactoeStates[i]) {
      case 0: //If light is not claimed
        changeLight(lightNum, 2, "on", "true", "bri", "100", "hue", "9000", "sat", "100");
        break;
      case 1: //If light is owned by blue player
        changeLight(lightNum, 2, "on", "true", "bri", "254", "hue", "44000", "sat", "200");
        break;
      case 2: //If light is owned by green player
        changeLight(lightNum, 2, "on", "true", "bri", "254", "hue", "24000", "sat", "200");
        break;
    }
  }
}


/* checkForVictory
   Check if a player has won the game
*/
void checkForVictory() {
  for (int i = 0; i < 8; i++) { //8 possible victory combinations - check each one
    //Player 1 = BLUE, hue = 44000
    //Player 2 = GREEN, hue = 2400
    int bluePlayerCount = 0; //If these get to 3 for a combination, this player wins
    int greenPlayerCount = 0;
    for (int lightNum : victoryCombinations[i]) { //For each of the 3 lights in this combination
      int owner = ownsLight(lightNum); //check who owns it
      if (owner == 1) bluePlayerCount++;
      if (owner == 2) greenPlayerCount++;
    }
    if (bluePlayerCount == 3) {
      changeGroup(1, 2, "on", "true", "bri", "20", "hue", "9000", "sat", "100"); // All lights dim
      for (int lightNum : victoryCombinations[i]) {
        changeLight(lightNum, 2, "on", "true", "bri", "150", "hue", "44000", "sat", "200");
      }
      delay(100);
      for (int j = 0; j < 10; j++) {
        for (int lightNum : victoryCombinations[i]) {
          changeLight(lightNum, 2, "on", "true", "bri", "254", "hue", "44000", "sat", "200");
          delay(200);
          changeLight(lightNum, 2, "on", "true", "bri", "150", "hue", "44000", "sat", "200");
        }
      }
      //changeGroup(1, 2, "on", "true", "bri", "254", "hue", "44000", "sat", "200");
      dbprintln("BLUE (1) WIN");
      //delay(5000);
      resetGame();
      //blue player WIN
    }
    if (greenPlayerCount == 3) {
      changeGroup(1, 2, "on", "true", "bri", "20", "hue", "9000", "sat", "100"); // All lights dim
      for (int lightNum : victoryCombinations[i]) {
        changeLight(lightNum, 2, "on", "true", "bri", "150", "hue", "24000", "sat", "200");
      }
      delay(100);
      for (int j = 0; j < 10; j++) {
        for (int lightNum : victoryCombinations[i]) {
          changeLight(lightNum, 2, "on", "true", "bri", "254", "hue", "24000", "sat", "200");
          delay(200);
          changeLight(lightNum, 2, "on", "true", "bri", "150", "hue", "24000", "sat", "200");
        }
      }
      //changeGroup(1, 2, "on", "true", "bri", "254", "hue", "24000", "sat", "200");   
      dbprintln("GREEN (2) WIN");         
      //delay(5000);
      resetGame();
      //green player WIN
    }
  }
}

/* checkForFullBoard
   checks if there are any spaces left
   on the board
   If there aren't, reset the game
*/
void checkForFullBoard() {
  bool anySpacesLeft = false;
  for (int positionStatus : tictactoeStates) {
    if (positionStatus == 0) anySpacesLeft = true;
  }
  if (!anySpacesLeft) {
    dbprintln("Game locked up - resetting");
    delay(2500);
    resetGame();
  }
}

/* ownsLight
   returns an integer representing the status of this light
   0 = unclaimed
   1 = owned by BLUE player
   2 = owned by GREEN player
*/
int ownsLight(int lightNum) {
  for (int i = 0; i < 9; i++) {
    if (lightIndexes[i] == lightNum) return tictactoeStates[i];
  }
}


/* lightNumToIndex
   Convert the number of a light in the Philips Hue system
   to it's index in the tic tac toe game (0-8)
*/
int lightNumToIndex(int lightNum) {
  for (int i = 0; i < 9; i++) {
    if (lightIndexes[i] == lightNum) return i;
  }
}


/* resetGame
   reset the variables that drive the tic tac toe game to restart
*/
void resetGame() {
  playerTakingTurn = 1;
  for (int i = 0; i < 9; i++) {
    tictactoeStates[i] = 0;
  }
  if(GAME_RUNNING) changeGroup(1, 2, "on", "true", "bri", "100", "hue", "9000", "sat", "100"); //Reset to default color
  GAME_RUNNING = false;
  //MQTT - SEND STATE 0 (OFF)
  sendState(0);
}

/* printGameState
   Print the states of the 9 "lights" in a 3x3 grid
   to play tic tac toe in the serial monitor
   Displays which player's turn it is
*/
void printGameState() {
  Serial.println("Player's Turn: " + String(playerTakingTurn));
  Serial.println("Game Status:");
  Serial.println(String(tictactoeStates[0]) + "  " + String(tictactoeStates[1]) + "  " + String(tictactoeStates[2]));
  Serial.println(String(tictactoeStates[3]) + "  " + String(tictactoeStates[4]) + "  " + String(tictactoeStates[5]));
  Serial.println(String(tictactoeStates[6]) + "  " + String(tictactoeStates[7]) + "  " + String(tictactoeStates[8]));
}


void setup_wifi() {
  WiFi.begin(ssid, password);
  dbprint("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    dbprint(".");
  }
  dbprintln("Connected to the WiFi network");
}

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
