enum class State;
void updateTypedString();
State getCurrentState();

bool saveMapToFlash();
bool loadMapFromFlash();

bool textInputExpected();

void readPrintInit();
void readPrint();

inline bool idPrompted;
inline bool idTyped;
inline int idToRegister;
inline bool firstScanPrompted;
inline bool firstScanComplete;
inline bool secondScanComplete;
inline bool secondScanPrompted;
void registerPrintInit();
void registerPrint();

inline bool idRemovalPrompted;
inline bool idToRemoveGiven;
inline int idToRemove;
inline bool confirmationPrompted;
inline bool confirmationGiven;
void removePrintInit();
void removePrint();