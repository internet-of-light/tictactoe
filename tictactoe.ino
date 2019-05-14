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

#define TEXT_TESTING_MODE true //Disable lights and wifi, play tic tac toe in serial monitor
#define DEBUG true //View button presses and Hue API calls in serial monitor

// Wifi network SSID
const char* ssid = "University of Washington";
// Wifi network password
const char* password = "";

// IP of Hue gateway
String ip = "172.28.219.179";
String api_token = "rARKEpLebwXuW01cNVvQbnDEkd2bd56Nj-hpTETB";



//Sieg Lower Floor
int buttonPins[] = {4, 16, 14, 5, 0, 12, 2, 15, 13};
int lightIndexes[] = {10, 23, 11, 15, 7, 14, 22, 21, 16};

//All possible combinations of 3 lights that result in a victory
int victoryCombinations[][3] = {{10, 15, 22}, {10, 7, 16}, {10, 23, 11}, {23, 7, 21}, {11, 7, 22}, {11, 14, 16}, {15, 7, 14}, {16, 21, 22}};

//0 = unclaimed
//1 = player 1
//2 = player 2
int tictactoeStates[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};


//always 1 or 2
int playerTakingTurn = 1;

int buttonToggles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; //For debouncing

//timing
unsigned long currentTime, buttonLastPressTime, lightLastUpdate;



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




/*  setup
 *  Initialie button pins and serial communication and connect to WiFi
 */
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  for (int pin : buttonPins) {
    if (pin == 15 or pin == 16) pinMode(pin, INPUT);
    else pinMode(pin, INPUT_PULLUP);
  }
  if (!TEXT_TESTING_MODE) {
    dbprint("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      dbprint(".");
    }
    dbprintln("Connected to the WiFi network");
  }
  buttonLastPressTime = 0;
}


/*  loop
 *  check buttons and run tic tac toe game
 */
void loop() {
  currentTime = millis(); //Update time (for debouncing)


  if (currentTime - buttonLastPressTime > 30000) {
    dbprintln("Timeout - resetting game");
    buttonLastPressTime = currentTime;
    resetGame();
  }

  checkButtons(); //Check buttons and trigger actions

  if (currentTime - lightLastUpdate > 1000) { //Update lights once per second
    if (TEXT_TESTING_MODE) {
      printGameState(); //Instead of pushing to lights, view game status in serial monitor
    } else {
      drawLights(); //push game information to Philips Hue lights
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
      dbprintln("Button " + String(i) + " pressed.");
      ttcManager(lightIndexes[i]);
      buttonToggles[i] = 1; //set buttonToggle to 1 so that we know that this button isn't being held down
      buttonLastPressTime = currentTime; //Stop player from accidentally taking next players turn by hitting 2 buttons fast
    }
  }
}





/* ttc (tic tac toe) Manager
   Called when a button is pressed
   Updates the state of the corresponding
   light if it was a valid move
*/
void ttcManager(int lightNum) {
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

      if (TEXT_TESTING_MODE) {
        dbprintln("BLUE (1) WIN");
        delay(5000);
      }

      resetGame();
      //blue player WIN
    }
    if (greenPlayerCount == 3) {

      if (TEXT_TESTING_MODE) {
        dbprintln("GREEN (2) WIN");
        delay(5000);
      }

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
 * Convert the number of a light in the Philips Hue system
 * to it's index in the tic tac toe game (0-8)
 */
int lightNumToIndex(int lightNum) {
  for (int i = 0; i < 9; i++) {
    if (lightIndexes[i] == lightNum) return i;
  }
}


/* resetGame
 * reset the variables that drive the tic tac toe game to restart
 */
void resetGame() {
  playerTakingTurn = 1;
  for (int i = 0; i < 9; i++) {
    tictactoeStates[i] = 0;
  }
}

/* printGameState
 * Print the states of the 9 "lights" in a 3x3 grid
 * to play tic tac toe in the serial monitor
 * Displays which player's turn it is 
 */
void printGameState() {
  Serial.println("Player's Turn: " + String(playerTakingTurn));
  Serial.println("Game Status:");
  Serial.println(String(tictactoeStates[0]) + "  " + String(tictactoeStates[1]) + "  " + String(tictactoeStates[2]));
  Serial.println(String(tictactoeStates[3]) + "  " + String(tictactoeStates[4]) + "  " + String(tictactoeStates[5]));
  Serial.println(String(tictactoeStates[6]) + "  " + String(tictactoeStates[7]) + "  " + String(tictactoeStates[8]));
}
