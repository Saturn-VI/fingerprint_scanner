#include <Arduino.h>
#include "Adafruit_Fingerprint.h"
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_TCA8418.h"
#include "SerialUSB.h"
#include "api/String.h"
#include "Keyboard.h"
#include "main.h"
#include <map>

#define ROWS 4
#define COLS 4

char keymap[COLS][ROWS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

#define fingerprintSerial Serial1

Adafruit_LiquidCrystal lcd(0);
Adafruit_TCA8418 keypad;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerprintSerial);

enum class State {
  Reading,
  Registering,
  Removing
};

String str = "";
State lastState;
State curState = State::Reading;
// key: fingerprint id, value: id to type
std::map<int, int> idMap;

void setup() {
  Keyboard.begin();
  Serial.begin(115200);
  while(!Serial) {
    delay(10);
  }
  Serial.println("Serial connected.");

  // set up the LCD's number of rows and columns:
  if (!lcd.begin(16, 2)) {
    Serial.println("Could not init backpack. Check wiring.");
    while (1);
  }

  if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("keypad not found, check wiring & pullups!");
    while (1);
  }
  keypad.matrix(4, 4);
  keypad.flush();

  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1);
  }
  // Remove once fingerprints are saved to flash
  finger.emptyDatabase();

  Serial.println("LCD, keypad, & fingerprint sensor init'd.");

  readPrintInit();
}

void loop() {
  updateTypedString();
  lcd.setCursor(0, 1);
  lcd.print(str);

  lastState = curState;
  curState = getCurrentState();

  switch (curState) {
    case State::Reading:
      readPrintLoop();
      break;
    case State::Registering:
      registerPrintLoop();
      break;
    case State::Removing:
      removePrintLoop();
      break;
  }
}

void updateTypedString() {
  if (keypad.available() > 0) {
    int k = keypad.getEvent();
    // keys are 1, 2, 3, 4, etc (row 2 is 11, 12, 13, ...)
    // release just gives the key
    // press gives key + 128, hence 0x80
    bool pressed = k & 0x80;
    k &= 0x7f; // remove leading bit no matter what
    // k is now from 1, 2, 3, 4, 11, ..., 32, 33, 34
    int row = k / 10;
    int col = (k % 10) - 1;
    char character = keymap[row][col];
    if (pressed) {
      if (character == 'A') {
        if (str.length() > 0) {
          str.remove(str.length() - 1);
          // e.g. if current string length after removing is 5, we want to clear 5
          // because the current end index would be 4
          lcd.setCursor(str.length(), 1);
          lcd.print(" ");
        }
      } else {
        if (str.length() < 16) {
          str += keymap[row][col];
        }
      }
    }
  }
}

State getCurrentState() {
  // immediately quit as state change must have at least *, char, and #
  if (str.length() < 3) {
    return curState;
  }

  // has asterisk been pressed to indicate state change?
  if (!str.startsWith("*")) {
    return curState;
  }

  // has pound been pressed to indicate enter?
  if (!str.endsWith("#")) {
    return curState;
  }

  char commandChar = str.charAt(1);
  str = "";
  lcd.clear();
  switch (commandChar) {
    case '1':
      readPrintInit();
      return State::Reading;
    case '2':
      registerPrintInit();
      return State::Registering;
    case '3':
      removePrintInit();
      return State::Removing;
    default:
      return curState;
  }
}

void readPrintInit() {
  lcd.setCursor(0, 0);
  lcd.print("READ");
}

void readPrintLoop() {
  // if fingerprint, check
  // type id if match
  // error on screen if not
}

void registerPrintInit() {
  lcd.setCursor(0, 0);
  lcd.print("REGISTER");
}

void registerPrintLoop() {
  // wait for id to be typed
  // prompt for fingerprint
  // get first scan
  // prompt again
  // scan again
  // save
}

void removePrintInit() {
  lcd.setCursor(0, 0);
  lcd.print("REMOVE");
}

void removePrintLoop() {
  // prompt to delete by id or fingerprint
  // ask for id or fingerprint
  // delete key & value
}