#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// ==================================================
// 1. PIN CONFIGURATION
// ==================================================

// RC522
#define SS_PIN  5
#define RST_PIN 4

// OLED
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

// BUZZER
#define BUZZER_PIN 25

// Active-HIGH buzzer configuration

#define BUZZER_ACTIVE_LEVEL HIGH
#define BUZZER_INACTIVE_LEVEL LOW


// ==================================================
// 2. RFID, OLED AND MEMORY OBJECTS
// ==================================================

MFRC522 rfid(SS_PIN, RST_PIN);

Preferences preferences;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

bool oledReady = false;

uint8_t oledAddress = 0;


// ==================================================
// 3. RFID DATABASE
// ==================================================

const byte UID_LENGTH = 4;


// BOOK UIDs

const byte BOOK_UIDS[2][UID_LENGTH] = {

  {0xFD, 0x32, 0x8F, 0xD0},  // Book 1
  {0x77, 0x18, 0x75, 0x06}   // Book 2

};


// STUDENT UIDs

const byte STUDENT_UIDS[2][UID_LENGTH] = {

  {0xB1, 0x83, 0x19, 0x66},  // Student 1
  {0x35, 0x46, 0xF6, 0x05}   // Student 2

};


// NAMES if you want to change the student names or book names, change in BOOK_NAMES ,STUDENT_NAMES, STUDENT_DISPLAY_NAMES

const char* BOOK_NAMES[2] = {

  "Book 1",
  "Book 2"

};

const char* STUDENT_NAMES[2] = {

  "student 1",
  "student 2"

};


// OLED display names

const char* STUDENT_DISPLAY_NAMES[2] = {

  "STUDENT 1",
  "STUDENT 2"

};


// ==================================================
// 4. BOOK STATUS
// ==================================================

// 255 = Available
// 0 = Issued to Student 1
// 1 = Issued to Student 2

const uint8_t AVAILABLE = 255;

uint8_t issuedTo[2] = {

  AVAILABLE,
  AVAILABLE

};


// ==================================================
// 5. SYSTEM STATES
// ==================================================

enum SystemState {

  WAIT_FOR_STUDENT,
  WAIT_FOR_BOOK

};

SystemState currentState = WAIT_FOR_STUDENT;

int8_t selectedStudent = -1;


// ==================================================
// 6. STUDENT TIMEOUT
// ==================================================

unsigned long bookWaitStartTime = 0;

const unsigned long BOOK_SCAN_TIMEOUT = 15000UL;


// ==================================================
// 7. BUZZER FUNCTIONS
// ==================================================


// One short beep for EVERY RFID scan

void beepScan() {

  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LEVEL);

  delay(70);

  digitalWrite(BUZZER_PIN, BUZZER_INACTIVE_LEVEL);

}


// One long beep for timeout

void beepTimeout() {

  digitalWrite(BUZZER_PIN, BUZZER_ACTIVE_LEVEL);

  delay(500);

  digitalWrite(BUZZER_PIN, BUZZER_INACTIVE_LEVEL);

}


// ==================================================
// 8. OLED FIXED-SCREEN DISPLAY FUNCTION
// ==================================================

void drawOLEDLine(const String& message, int y) {

  String text = message;

  // Maximum 21 characters per line

  if (text.length() > 21) {

    text = text.substring(0, 21);

  }

  display.setCursor(0, y);

  display.print(text);

}


// Display heading and four short lines.
// No scrolling. No automatic wrapping.

void showOLED(
  const String& title,
  const String& line1,
  const String& line2,
  const String& line3,
  const String& line4
) {

  if (!oledReady) {
    return;
  }

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(SSD1306_WHITE);

  display.setTextWrap(false);


  // HEADER

  drawOLEDLine(title, 1);


  // HEADER SEPARATOR

  display.drawFastHLine(
    0,
    12,
    SCREEN_WIDTH,
    SSD1306_WHITE
  );


  // FIXED BODY LINES

  drawOLEDLine(line1, 17);

  drawOLEDLine(line2, 28);

  drawOLEDLine(line3, 39);

  drawOLEDLine(line4, 50);


  display.display();

}


// ==================================================
// 9. FIND OLED I2C ADDRESS
// ==================================================

uint8_t findOLEDAddress() {

  const uint8_t addresses[] = {

    0x3C,
    0x3D

  };

  for (byte i = 0; i < 2; i++) {

    Wire.beginTransmission(addresses[i]);

    if (Wire.endTransmission() == 0) {

      return addresses[i];

    }

  }

  return 0;

}


// ==================================================
// 10. INITIALIZE OLED
// ==================================================

void initializeOLED() {

  Serial.println("Initializing OLED...");


  Wire.begin(
    OLED_SDA_PIN,
    OLED_SCL_PIN
  );


  delay(50);


  oledAddress = findOLEDAddress();


  if (oledAddress == 0) {

    Serial.println("ERROR: OLED not detected!");

    oledReady = false;

    return;

  }


  Serial.print("OLED Address: 0x");

  Serial.println(oledAddress, HEX);


  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        oledAddress
      )) {

    Serial.println("OLED initialization failed!");

    oledReady = false;

    return;

  }


  oledReady = true;


  Serial.println("OLED initialized successfully!");


  showOLED(
    "LIBRARY SYSTEM",
    "OLED READY",
    "",
    "",
    ""
  );

}


// ==================================================
// 11. RFID UID COMPARISON
// ==================================================

bool uidMatches(
  const byte expectedUID[],
  byte length
) {

  if (rfid.uid.size != length) {

    return false;

  }


  for (byte i = 0; i < length; i++) {

    if (
      rfid.uid.uidByte[i] != expectedUID[i]
    ) {

      return false;

    }

  }

  return true;

}


// ==================================================
// 12. IDENTIFY STUDENT
// ==================================================

int8_t findStudent() {

  for (byte i = 0; i < 2; i++) {

    if (
      uidMatches(
        STUDENT_UIDS[i],
        UID_LENGTH
      )
    ) {

      return i;

    }

  }

  return -1;

}


// ==================================================
// 13. IDENTIFY BOOK
// ==================================================

int8_t findBook() {

  for (byte i = 0; i < 2; i++) {

    if (
      uidMatches(
        BOOK_UIDS[i],
        UID_LENGTH
      )
    ) {

      return i;

    }

  }

  return -1;

}


// ==================================================
// 14. SAVE BOOK STATUS
// ==================================================

bool saveBookStatus(
  byte bookIndex,
  uint8_t borrower
) {

  const char* key =
    (bookIndex == 0) ? "book1" : "book2";


  size_t result = preferences.putUChar(
    key,
    borrower
  );


  return result == 1;

}


// ==================================================
// 15. LOAD BOOK STATUS
// ==================================================

void loadBookStatus() {

  issuedTo[0] = preferences.getUChar(
    "book1",
    AVAILABLE
  );

  issuedTo[1] = preferences.getUChar(
    "book2",
    AVAILABLE
  );


  for (byte i = 0; i < 2; i++) {

    if (
      issuedTo[i] != AVAILABLE &&
      issuedTo[i] >= 2
    ) {

      Serial.print("Invalid saved status: ");

      Serial.println(BOOK_NAMES[i]);


      issuedTo[i] = AVAILABLE;

      saveBookStatus(i, AVAILABLE);

    }

  }

}


// ==================================================
// 16. DISPLAY BOOK DATABASE
// ==================================================

void displayBookStatus() {

  Serial.println();

  Serial.println("SAVED LIBRARY DATABASE");


  for (byte i = 0; i < 2; i++) {

    Serial.print(BOOK_NAMES[i]);

    Serial.print(" : ");


    if (issuedTo[i] == AVAILABLE) {

      Serial.println("AVAILABLE");

    }

    else if (issuedTo[i] < 2) {

      Serial.print("ISSUED TO ");

      Serial.println(STUDENT_NAMES[issuedTo[i]]);

    }

  }


  Serial.println("--------------------------------");

}


// ==================================================
// 17. SCAN STUDENTS
// ==================================================

void scanStudents() {

  currentState = WAIT_FOR_STUDENT;

  selectedStudent = -1;


  Serial.println();

  Serial.println("==============================");

  Serial.println("scan students...");

  Serial.println("==============================");


  showOLED(
    "LIBRARY SYSTEM",
    "SCAN STUDENT",
    "Present your card",
    "",
    ""
  );

}


// ==================================================
// 18. SCAN BOOK
// ==================================================

void scanBook() {

  currentState = WAIT_FOR_BOOK;


  Serial.println();

  Serial.print("student: ");

  Serial.print(STUDENT_NAMES[selectedStudent]);

  Serial.println("; scan book...");


  showOLED(
    STUDENT_DISPLAY_NAMES[selectedStudent],
    "SCAN BOOK",
    "15 SEC TO SCAN",
    "",
    ""
  );

}


// ==================================================
// 19. STUDENT MODE ERROR
// ==================================================

void studentError(
  const char* serialMessage,
  const char* oledMessage
) {

  Serial.print("ERROR: ");

  Serial.println(serialMessage);


  showOLED(
    "SCAN ERROR",
    oledMessage,
    "TRY AGAIN",
    "",
    ""
  );


  delay(2000);


  scanStudents();

}


// ==================================================
// 20. BOOK MODE ERROR
// ==================================================

void bookError(
  const char* serialMessage,
  const char* oledMessage
) {

  Serial.print("ERROR: ");

  Serial.println(serialMessage);


  showOLED(
    "SCAN ERROR",
    oledMessage,
    "TRY AGAIN",
    "",
    ""
  );


  delay(2000);


  // Keep same student session

  scanBook();

}


// ==================================================
// 21. ISSUE BOOK
// ==================================================

void issueBook(byte bookIndex) {


  if (
    saveBookStatus(
      bookIndex,
      selectedStudent
    )
  ) {

    issuedTo[bookIndex] = selectedStudent;


    Serial.println();

    Serial.print(BOOK_NAMES[bookIndex]);

    Serial.print(", issued to ");

    Serial.println(STUDENT_NAMES[selectedStudent]);


    showOLED(
      "BOOK ISSUED",
      String(BOOK_NAMES[bookIndex]),
      String("TO ") + STUDENT_DISPLAY_NAMES[selectedStudent],
      "SUCCESS!",
      ""
    );


    delay(5000);


    scanStudents();

  }

  else {

    bookError(
      "Unable to save book status.",
      "MEMORY SAVE FAILED"
    );

  }

}


// ==================================================
// 22. RETURN BOOK
// ==================================================

void returnBook(byte bookIndex) {


  if (
    saveBookStatus(
      bookIndex,
      AVAILABLE
    )
  ) {

    issuedTo[bookIndex] = AVAILABLE;


    Serial.println();

    Serial.print(BOOK_NAMES[bookIndex]);

    Serial.print(", returned by ");

    Serial.println(STUDENT_NAMES[selectedStudent]);


    showOLED(
      "BOOK RETURNED",
      String(BOOK_NAMES[bookIndex]),
      String("BY ") + STUDENT_DISPLAY_NAMES[selectedStudent],
      "SUCCESS!",
      ""
    );


    delay(5000);


    scanStudents();

  }

  else {

    bookError(
      "Unable to save book status.",
      "MEMORY SAVE FAILED"
    );

  }

}


// ==================================================
// 23. SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  Serial.println();

  Serial.println("================================");

  Serial.println("RFID LIBRARY MANAGEMENT SYSTEM");

  Serial.println("OLED AND BUZZER VERSION");

  Serial.println("================================");


  // BUZZER INITIALIZATION

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(
    BUZZER_PIN,
    BUZZER_INACTIVE_LEVEL
  );


  // OLED INITIALIZATION

  initializeOLED();


  // RFID SPI INITIALIZATION

  SPI.begin(
    18,
    19,
    23,
    SS_PIN
  );


  rfid.PCD_Init();

  delay(4);


  // CHECK RFID READER

  byte version =
    rfid.PCD_ReadRegister(MFRC522::VersionReg);


  if (
    version == 0x00 ||
    version == 0xFF
  ) {

    Serial.println("ERROR: RFID reader not detected!");


    showOLED(
      "HARDWARE ERROR",
      "RC522 NOT FOUND",
      "CHECK CONNECTIONS",
      "",
      ""
    );


    while (true) {

      delay(1000);

    }

  }


  Serial.println("RFID Reader Initialized!");


  // MEMORY INITIALIZATION

  if (!preferences.begin("library", false)) {

    Serial.println("ERROR: Memory initialization failed!");


    showOLED(
      "MEMORY ERROR",
      "STORAGE FAILED",
      "CHECK ESP32",
      "",
      ""
    );


    while (true) {

      delay(1000);

    }

  }


  // LOAD SAVED BOOK DATA

  loadBookStatus();


  displayBookStatus();


  Serial.println("SYSTEM READY!");


  showOLED(
    "LIBRARY SYSTEM",
    "INITIALIZATION OK",
    "SYSTEM READY",
    "",
    ""
  );


  delay(1000);


  // START SCANNING STUDENTS

  scanStudents();

}


// ==================================================
// 24. MAIN LOOP
// ==================================================

void loop() {


  // ================================================
  // CHECK 15-SECOND TIMEOUT
  // ================================================

  if (currentState == WAIT_FOR_BOOK) {

    if (
      millis() - bookWaitStartTime >=
      BOOK_SCAN_TIMEOUT
    ) {

      Serial.println();

      Serial.println("================================");

      Serial.println("TIMEOUT!");

      Serial.println("No book scanned in 15 seconds.");

      Serial.println("Student session cancelled.");

      Serial.println("================================");


      showOLED(
        "SESSION TIMEOUT",
        "NO BOOK SCANNED",
        "SESSION ENDED",
        "SCAN STUDENT AGAIN",
        ""
      );


      // Long beep for timeout

      beepTimeout();


      delay(2000);


      scanStudents();


      return;

    }

  }


  // ================================================
  // CHECK FOR RFID CARD
  // ================================================

  if (!rfid.PICC_IsNewCardPresent()) {

    return;

  }


  if (!rfid.PICC_ReadCardSerial()) {

    return;

  }


  // ================================================
  // RFID SCAN ACKNOWLEDGEMENT
  // ================================================

  // One short beep for every successfully read RFID tag.
  // Works for student, book and unknown RFID cards.

  beepScan();


  // ================================================
  // IDENTIFY SCANNED CARD
  // ================================================

  int8_t studentIndex = findStudent();

  int8_t bookIndex = findBook();


  // PRINT UID

  Serial.print("Scanned UID: ");


  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {

      Serial.print("0");

    }

    Serial.print(
      rfid.uid.uidByte[i],
      HEX
    );


    if (i < rfid.uid.size - 1) {

      Serial.print(":");

    }

  }

  Serial.println();


  // STOP RFID COMMUNICATION

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();


  // ================================================
  // MODE 1: WAIT FOR STUDENT
  // ================================================

  if (currentState == WAIT_FOR_STUDENT) {


    // STUDENT SCANNED

    if (studentIndex >= 0) {

      selectedStudent = studentIndex;


      // Start 15-second timer

      bookWaitStartTime = millis();


      Serial.println("STUDENT IDENTIFIED!");


      scanBook();

    }


    // BOOK SCANNED INSTEAD

    else if (bookIndex >= 0) {

      studentError(
        "Please scan a student card, not a book.",
        "STUDENT CARD NEEDED"
      );

    }


    // UNKNOWN RFID

    else {

      studentError(
        "Unregistered RFID card.",
        "UNKNOWN RFID TAG"
      );

    }

  }


  // ================================================
  // MODE 2: WAIT FOR BOOK
  // ================================================

  else if (currentState == WAIT_FOR_BOOK) {


    // STUDENT SCANNED INSTEAD OF BOOK

    if (studentIndex >= 0) {

      bookError(
        "Please scan a book card, not a student card.",
        "BOOK CARD NEEDED"
      );

    }


    // UNKNOWN RFID

    else if (bookIndex < 0) {

      bookError(
        "Unregistered RFID card.",
        "UNKNOWN RFID TAG"
      );

    }


    // VALID BOOK SCANNED

    else {


      // CASE 1: BOOK AVAILABLE

      if (issuedTo[bookIndex] == AVAILABLE) {

        issueBook(bookIndex);

      }


      // CASE 2: BOOK ISSUED TO SAME STUDENT

      else if (
        issuedTo[bookIndex] == selectedStudent
      ) {

        returnBook(bookIndex);

      }


      // CASE 3: BOOK ISSUED TO ANOTHER STUDENT

      else if (issuedTo[bookIndex] < 2) {


        String errorMessage =
          String(BOOK_NAMES[bookIndex]);

        errorMessage += " is already issued to ";

        errorMessage += STUDENT_NAMES[issuedTo[bookIndex]];


        Serial.println();

        Serial.print("ERROR: ");

        Serial.println(errorMessage);


        showOLED(
          "BOOK UNAVAILABLE",
          BOOK_NAMES[bookIndex],
          String("ISSUED TO ") +
            STUDENT_DISPLAY_NAMES[issuedTo[bookIndex]],
          "TRY ANOTHER BOOK",
          ""
        );


        delay(2000);


        // Keep current student selected

        scanBook();

      }

    }

  }

}
