#include <Arduino.h>

enum class State;
void updateTypedString();
State getCurrentState();

boolean textInputExpected();

void readPrintInit();
void readPrint();

inline boolean idPrompted;
inline boolean idTyped;
inline int idToRegister;
inline boolean firstScanPrompted;
inline boolean firstScanComplete;
inline boolean secondScanComplete;
inline boolean secondScanPrompted;
void registerPrintInit();
void registerPrint();

void removePrintInit();
void removePrint();