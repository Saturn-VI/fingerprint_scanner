#include <Arduino.h>
#include "Adafruit_Fingerprint.h"
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_TCA8418.h"
#include "SerialUSB.h"
#include "api/String.h"
#include "Keyboard.h"
#include "main.h"
#include <charconv>
#include <map>
#include <string>
#include <system_error>

#define ROWS 4
#define COLS 4
#define SIXTEEN_SPACES "                "

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
boolean inputEnabled = true;
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
      readPrint();
      break;
    case State::Registering:
      registerPrint();
      break;
    case State::Removing:
      removePrint();
      break;
  }
}

int getFirstOpenID() {
  if (idMap.empty()) {
    return 0;
  }

  int prevKey = idMap.begin()->first;
  for (const auto& [key, value] : idMap) {
    if (key - prevKey > 1) {
      return key - 1;
    }
    prevKey = key;
  }
  // id range up to end of map is packed
  // so we get the last index plus one
  // which is equivalent to the size of the id map
  return idMap.size();
}

void clearInput() {
  str = "";
  lcd.setCursor(0, 1);
  lcd.print(SIXTEEN_SPACES);
}

void updateTypedString() {
  if (!inputEnabled) {
    keypad.flush();
    return;
  }

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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("READ");
  clearInput();
  inputEnabled = true;
}

void readPrint() {
  // if fingerprint, check
  // type id if match
  // error on screen if not
}

boolean idPrompted;
boolean idTyped;
int idToRegister;
boolean firstScanPrompted;
boolean firstScanComplete;
boolean secondScanPrompted;
boolean secondScanComplete;

void registerPrintInit() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REGISTER");
  clearInput();
  inputEnabled = true;
  idPrompted = false;
  idTyped = false;
  idToRegister = 0;
  firstScanPrompted = false;
  firstScanComplete = false;
  secondScanPrompted = false;
  secondScanComplete = false;
}

void registerPrint() {
  // wait for id to be typed
  if (!idTyped) {
    if (!idPrompted) {
      clearInput();
      lcd.setCursor(0, 1);
      lcd.print("Enter ID");
      delay(1500);
      clearInput();
      idPrompted = true;
    }
    if (!str.endsWith("#")) {
      return;
    }
    String sub = str.substring(0, str.indexOf("#"));
    const char* start = sub.c_str();
    const char* end = start + sub.length();
    int value = 0;
    std::from_chars_result fCR = std::from_chars(start, end, value);
    if (fCR.ec == std::errc::invalid_argument ||
      fCR.ec == std::errc::result_out_of_range ||
      value == 0
    ) {
      clearInput();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      lcd.setCursor(0, 1);
      lcd.print("ID parse failed");
      delay(1500);
      registerPrintInit();
      return;
    } else {
      clearInput();
      lcd.setCursor(0, 1);
      lcd.print(("read ID " + std::to_string(value)).c_str());
      delay(1500);
      clearInput();
    }
    idTyped = true;
    idToRegister = value;
    inputEnabled = false;
  }

  // prompt for fingerprint
  if (!firstScanComplete) {
    if (!firstScanPrompted) {
      clearInput();
      lcd.setCursor(0, 1);
      lcd.print("Scan finger");
      firstScanPrompted = true;
    }

    // get first scan
    int p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      return;
    }
    if (p != FINGERPRINT_OK) {
      clearInput();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      lcd.setCursor(0, 1);
      lcd.print("Imaging failed");
      delay(1500);
      registerPrintInit();
      return;
    }
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
      clearInput();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      lcd.setCursor(0, 1);
      lcd.print("Hashing failed");
      delay(1500);
      registerPrintInit();
      return;
    }
    // first scan done!
    clearInput();
    lcd.setCursor(0, 1);
    lcd.print("Remove finger");
    delay(1500);
    clearInput();
    firstScanComplete = true;
  }

  // prompt again
  if (!secondScanComplete) {
    if (!secondScanPrompted) {
      clearInput();
      lcd.setCursor(0, 1);
      lcd.print("Scan again");
      secondScanPrompted = true;
    }

    // scan again
    int p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      return;
    }
    if (p != FINGERPRINT_OK) {
      clearInput();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      lcd.setCursor(0, 1);
      lcd.print("Imaging failed");
      delay(1500);
      registerPrintInit();
      return;
    }
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
      clearInput();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      lcd.setCursor(0, 1);
      lcd.print("Hashing failed");
      delay(1500);
      registerPrintInit();
      return;
    }
    // second scan done!
    clearInput();
    lcd.setCursor(0, 1);
    lcd.print("Remove finger");
    delay(1500);
    clearInput();
    secondScanComplete = true;
  }

  // save
  int p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    clearInput();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Bad print match");
    delay(2000);
    registerPrintInit();
    return;
  }

  int indexId = getFirstOpenID();
  p = finger.storeModel(indexId);
  if (p != FINGERPRINT_OK) {
    clearInput();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
    lcd.setCursor(0, 1);
    lcd.print("Storing failed");
    delay(2000);
    registerPrintInit();
    return;
  }
  idMap[indexId] = idToRegister;
  clearInput();
  lcd.setCursor(0, 1);
  lcd.print("Saved print!");
  delay(2000);
  curState = State::Reading;
  readPrintInit();
  return;
}

void removePrintInit() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("REMOVE");
  clearInput();
  inputEnabled = true;
}

void removePrint() {
  // prompt to delete by id or fingerprint
  // ask for id or fingerprint
  // delete key & value
}