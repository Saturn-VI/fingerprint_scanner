#include <Arduino.h>
#include "Adafruit_LiquidCrystal.h"
#include "Adafruit_TCA8418.h"
#include "api/String.h"

Adafruit_LiquidCrystal lcd(0);
Adafruit_TCA8418 keypad;

#define ROWS 4
#define COLS 4

char keymap[COLS][ROWS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

String str = "";

void setup() {
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

  Serial.println("LCD & keypad init'd.");

  // Print a message to the LCD.
  lcd.print("hello, world!");
}

void loop() {
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
      Serial.print("pressed ");
      Serial.println(character);
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
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print(str);
}