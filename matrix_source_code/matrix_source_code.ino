#include "LedControl.h"  // Include the LedControl library for controlling LED matrices
// Pin definitions for the LED matrix
#include <LiquidCrystal.h>
#include <EEPROM.h>
const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;
// Pin definitions for the joystick
const int xPin = A0;
const int yPin = A1;
const int swPin = A2;
// Create an LedControl object to interface with the LED matrix
LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);  // DIN, CLK, LOAD, number of devices
// Variables to control LED matrix brightness and position
byte matrixBrightness = 2;
byte xPos = 0;
byte yPos = 0;
byte xLastPos = 0;
byte yLastPos = 0;
// Thresholds for joystick movement detection
const int minThreshold = 200;
const int maxThreshold = 600;
// Timing variables for movement updates
const byte moveInterval = 500;
unsigned long lastMoved = 0;
// Size of the LED matrix (8x8)
const byte matrixSize = 8;
bool matrixChanged = true;
unsigned long swPressTime = 0;
byte swState = HIGH;
byte lastSwState = LOW;

// 2D array representing the state (on/off) of each LED in the matrix
byte matrix[matrixSize][matrixSize] = { /* Initial state with all LEDs off */ };

int explodeTime = 1000;  //one second to explode
bool gameStarted = false;

int currentMenuItem = 0;
int lastMenuItem = -1;
bool joyMoved = false;

unsigned long lastJoyMoveTime = 0;
const int debounceDelay = 250;

int currentMenuState = 0;
byte logo[] = {
  B00010,
  B00100,
  B01110,
  B11111,
  B11111,
  B01110,
  B00000,
  B00000
};
int timeElapsed = 0;
int LEVEL = 0;
int TIMER = 300000;
int SCORE = 0;
int MODIFIER = 0;
int THIGHSCORE = 0;

void setup() {
  pinMode(swPin, INPUT_PULLUP);
  //EEPROM.update(20,1);
  LEVEL = EEPROM.read(20);
  THIGHSCORE = EEPROM.read(21);
  lcd.begin(16, 2);
  lcd.clear();
 // Serial.begin(9600);
  lcd.createChar(0, logo);
  lcd.home();
  lcd.setCursor(2, 0);
  lcd.write((uint8_t)0);
  lcd.print("StressMaze");
  lcd.write((uint8_t)0);
  lcd.setCursor(4, 1);
  lcd.print("Meltdown");
  delay(2000);
  lcd.clear();
  show_menu();
}
void setup_game() {
  xPos = yPos = 0;
  lc.shutdown(0, false);                 // Disable power saving, turn on the display
  lc.setIntensity(0, matrixBrightness);  // Set the brightness level
  lc.clearDisplay(0);                    // Clear the display initially
  matrix[xPos][yPos] = 1;
  LEVEL = EEPROM.read(20);  // Turn on the initial LED position
  generateRandomWalls(matrix);
  updateMatrix();
  gameStarted = true;
  timeElapsed = millis();
}

void loop() {
  swState = digitalRead(swPin);
  if (currentMenuItem == 0 && gameStarted == false) {

    if (swState != lastSwState) {
      if (swState == LOW) {
        swPressTime = millis();
      } else if (millis() - swPressTime > 300) {
        gameStarted = true;
        lcd.clear();
        lcd.print("LEVEL");
        lcd.print(LEVEL);
        delay(100);
        setup_game();
      }
    }
    lastSwState = swState;
  }

  if (gameStarted) {
    in_game();
  } else {
    if (THIGHSCORE < SCORE) {
      THIGHSCORE = SCORE;
      EEPROM.update(21, THIGHSCORE);
    }
    joyMoved = false;
    show_menu();
  }
}
void generateRandomWalls(byte targetMatrix[matrixSize][matrixSize]) {
  randomSeed(analogRead(5));
  if (LEVEL == 1) {  //generate an easier map - walls only in the opposite corner of the player and a lot of time
    TIMER = 40000;  //30 sec
    MODIFIER = 1;
    for (int row = 4; row < matrixSize; row++) {
      for (int col = 4; col < matrixSize; col++)
        if (row == 0 && col == 0 || row == 1 && col == 0 || row == 0 && col == 1 || row == 1 && col == 1) {
          //dont generate on the starting point for the player and on the neighbour steps. i did this so that the player isn't stuck
        } else {
          // Generate random values (0 or 1)
          targetMatrix[row][col] = random(2);
        }
    }
  }
  if (LEVEL == 2) {
    MODIFIER = 10;
    TIMER = 45000;  //30 sec
    for (int row = 2; row < matrixSize; row++) {
      for (int col = 2; col < matrixSize; col++)
        if (row == 0 && col == 0 || row == 1 && col == 0 || row == 0 && col == 1 || row == 1 && col == 1) {
        } else {
          if (random(2))  // I thought it wasn't generating enough walls. so it randomly gets 0 or 1 - for 1 it does the random again, for 0 it puts a wall instantly
            targetMatrix[row][col] = random(2);
          else targetMatrix[row][col] = 1;
        }
    }
  }
  if (LEVEL == 3) {
    MODIFIER = 20;
    TIMER = 45000;  // 45 SECONDS. more walls. Made it so short to be hardcore and to prove the dying mechanic works xd
    for (int row = 1; row < matrixSize; row++) {
      for (int col = 1; col < matrixSize; col++)
        if (row == 0 && col == 0 || row == 1 && col == 0 || row == 0 && col == 1 || row == 1 && col == 1) {
        } else {
          // Generate random values (0 or 1)
          targetMatrix[row][col] = random(2);
        }
    }
  }
}

void updatePositions() {
  int xValue = analogRead(xPin);  // Read the X-axis value
  int yValue = analogRead(yPin);  // Read the Y-axis value
  // Store the last positions
  xLastPos = xPos;
  yLastPos = yPos;

  // Update xPos based on joystick movement
  if (xValue < minThreshold && matrix[(xPos + 1) % matrixSize][yPos] != 1) {
    xPos = (xPos + 1) % matrixSize;
  } else if (xValue > maxThreshold && matrix[xPos - 1][yPos] != 1) {
    xPos = (xPos > 0) ? xPos - 1 : matrixSize - 1;
  }
  // Update yPos based on joystick movement
  if (yValue < minThreshold && matrix[xPos][(yPos - 1) % matrixSize] != 1) {
    yPos = (yPos > 0) ? yPos - 1 : matrixSize - 1;
  } else if (yValue > maxThreshold && matrix[xPos][(yPos + 1) % matrixSize] != 1) {
    yPos = (yPos + 1) % matrixSize;
  }



  // Check if the position has changed and update the matrix accordingly
  if (xPos != xLastPos || yPos != yLastPos) {
    matrixChanged = true;
    matrix[xLastPos][yLastPos] = 0;  // Turn off the LED at the last position
    matrix[xPos][yPos] = 1;          // Turn on the LED at the new position
  }
}

void placebomb(int x, int y) {

  // Update the player. The bomb is placed in the current position of the player so they player is moved in an available neighbour empty space
  displayText("This is a dream", "Bombs = harmless");
  if (matrix[(xPos + 1) % matrixSize][yPos] != 1) {
    xPos = (xPos + 1) % matrixSize;
  } else if (matrix[xPos - 1][yPos] != 1) {
    xPos = (xPos > 0) ? xPos - 1 : matrixSize - 1;
  } else if (matrix[xPos][(yPos - 1) % matrixSize] != 1) {
    yPos = (yPos > 0) ? yPos - 1 : matrixSize - 1;
  } else if (matrix[xPos][(yPos + 1) % matrixSize] != 1) {
    yPos = (yPos + 1) % matrixSize;
  }
  if (xPos != xLastPos || yPos != yLastPos) {
    matrixChanged = true;
    matrix[xLastPos][yLastPos] = 2;  // place bomb
    matrix[xPos][yPos] = 1;          // Turn on the LED at the new position
    updateMatrix();
  }

  unsigned long startTime = millis();

  // Update the matrix to place the bomb
  matrix[x][y] = 2;  // Use a different value to represent the bomb (e.g., 2)

  while (millis() - startTime < 2000) {
    lc.setLed(0, x, y, millis() % 200 < 100);  // Blink the bomb LED
  }

  // Explode the bomb and update the matrix
  if (matrix[x + 1][y] == 1) {
    SCORE = SCORE + MODIFIER;
  }
  matrix[x + 1][y] = 0;

  if (matrix[x - 1][y] == 1) {
    SCORE = SCORE + MODIFIER;
  }
  matrix[x - 1][y] = 0;

  if (matrix[x][y + 1] == 1) {
    SCORE = SCORE + MODIFIER;
  }
  matrix[x][y + 1] = 0;

  if (matrix[x][y - 1] == 1) {
    SCORE = SCORE + MODIFIER;
  }
  matrix[x][y - 1] = 0;
  matrix[x][y] = 0;      // Turn off the LED at the bomb position
  matrixChanged = true;  // Matrix has changed due to bomb explosion
  explodeTime = 0;
  lcd.clear();
  lcd.print("SCORE: ");
  lcd.print(SCORE);
}


// Function to update the LED matrix display
void updateMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      if (matrix[row][col] == 2) {
        if (millis() < explodeTime) {
          // Blink the bomb fast
          lc.setLed(0, row, col, millis() % 200 < 100);
        } else {
          // Explode the bomb and turn off the LED
          matrix[row][col] = 0;
          lc.setLed(0, row, col, false);
          matrixChanged = true;  // Matrix has changed due to bomb explosion
        }
      } else {
        // Update other LEDs
        lc.setLed(0, row, col, matrix[row][col]);
      }
    }
  }
}

void show_menu() {
  if (!gameStarted) {
    int xValue = analogRead(xPin);  // Read the X-axis value
    if (millis() - lastJoyMoveTime > debounceDelay) {
      if (xValue < minThreshold && currentMenuItem != 0 && joyMoved == false) {
        currentMenuItem = (currentMenuItem - 1 + 5) % 5;
        joyMoved = true;
        lastJoyMoveTime = millis();
      }
      if (xValue > maxThreshold && currentMenuItem != 4 && joyMoved == false) {
        currentMenuItem = (currentMenuItem + 1) % 5;
        joyMoved = true;
        lastJoyMoveTime = millis();
      }
    }
    if (joyMoved) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }

    if (lastMenuItem != currentMenuItem)
      switch (currentMenuItem) {
        case 0:
          lcd.print("Start Game");
          break;
        case 1:
          lcd.print("Highscore");
          break;
        case 2:
          lcd.print("Settings");
          break;
        case 3:
          lcd.print("About");
          break;
        case 4:
          lcd.print("HOW TO PLAY");
          break;
      }
    swState = digitalRead(swPin);
    if (swState != lastSwState)
      if (swState == LOW) {
        swPressTime = millis();
      } else if (millis() - swPressTime > 300 && millis() - swPressTime < 800) {
        handleSubMenu(currentMenuItem);
      }
    joyMoved = false;
    lastMenuItem = currentMenuItem;
    lastSwState = swState;
  }
}

void handleSubMenu(int currentMenuState) {
  switch (currentMenuState) {
    case 0:
      gameStarted = true;
      break;
    case 1:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(THIGHSCORE);
      swState = digitalRead(swPin);
      if (swState != lastSwState){
      if (swState == LOW) {swPressTime = millis();}
      else
      if(millis()-swPressTime > 1000)
      {
        THIGHSCORE = 0;
        EEPROM.update(21, 0);
        lcd.clear(); 
        lcd.print("reset highscore");
        delay(200);
        lcd.clear();
        lcd.print(THIGHSCORE);
      lastSwState = swState;}}
      break;
    case 2:
      displayText("WORK IN ", "PROGRESS");
      break;
    case 3:

      displayText("A BOMBERMAN BY", "Steica Malina");
      break;
    case 4:
      displayText("MOVE JOYSTICK ><", "PRESS -> BOMB");
      lcd.write((uint8_t)0);
      break;
  }
}

void displayText(const char *line1, const char *line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  delay(100);
}

void in_game() {
  if (millis() - lastMoved > moveInterval) {
    updatePositions();     // Update the LED position based on joystick input
    lastMoved = millis();  // Reset the movement timer
  }
  // Update the LED matrix display if there's been a change
  if (matrixChanged) {
    updateMatrix();

    matrixChanged = false;
  }

  lc.setLed(0, xPos, yPos, millis() % 1000 < 250);  //make the player blink slowly every second

  swState = digitalRead(swPin);
  if (swState != lastSwState) {
    if (swState == LOW) {
      swPressTime = millis();
    } else {
      if (millis() - swPressTime < 500) {  //this is so that nothing happens when you keep the button pressed
        placebomb(xPos, yPos);
        updateMatrix();
      }
    }
    lastSwState = swState;
  }

  for (int row = 0; row < matrixSize; row++)
    for (int col = 0; col < matrixSize; col++)
      if (matrix[row][col] == 2) {
        lc.setLed(0, row, col, millis() % 200 < 250);
      }
  check_win();
  if (millis() - timeElapsed > TIMER)
    game_over();
}

void game_over() {
  check_win();
  lc.clearDisplay(0);
  displayText("GAME", "OVER");
  delay(1000);
  lcd.clear();
  displayText("You missed", "your alarm");
  delay(1000);
  lcd.clear();
  displayText("You are fired", ":(");
  delay(500);
  gameStarted = false;
  show_menu();
}

void check_win() {

  int no1 = 0;  // 1 led should be on at the end - the player

  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      if (matrix[row][col] == 1)
        no1++;
    }
  }
  if(SCORE!=0)
    {if (no1 == 1 || no1 == 0)
      WIN();}
  else {
  gameStarted = true;
  setup_game();
  }//THIS IS TO SOLVE A BUG WHEN YOU FIRST OPEN THE GAME. IDK
  
}

void WIN() {
  if (SCORE > THIGHSCORE) {
    displayText("!You beat the", "HIGHSCORE!");
    delay(1000);
    lcd.clear();
    lcd.print("YOUR SCORE ");
    lcd.print(SCORE);
    delay(1000);
    THIGHSCORE = SCORE;
    EEPROM.update(21, THIGHSCORE);
    lcd.clear();
  }

  if (LEVEL == 1 && millis() - timeElapsed > 1000) {
    displayText("CONGRATS!", "NIGHT 2...");
    EEPROM.update(20, 2);
    setup_game();
  } else if (LEVEL == 2 && millis() - timeElapsed > 1000) {
    displayText("CONGRATS!", "NIGHT 3...");
    EEPROM.update(20, 3);
    setup_game();
  } else if (LEVEL == 3 && millis() - timeElapsed > 300) {
    displayText("BUZZ", "???");
    delay(600);
    lcd.clear();
    displayText("YOU ESCAPED", "YOU WON!!");
    delay(800);
    lcd.clear();
    displayText("CONGRATULATIONS", "<3");
    LEVEL = 1;
    EEPROM.update(20, 1);  //back to level 1
    lc.clearDisplay(0);
    gameStarted = false;
  }
}
