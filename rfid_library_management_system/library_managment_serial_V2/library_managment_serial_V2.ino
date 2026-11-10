
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>

// ==========================================
// 1. RFID CONNECTION PINS
// ==========================================

#define SS_PIN  5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

Preferences preferences;


// ==========================================
// 2. REGISTERED RFID UID DATABASE
// ==========================================

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

// Registered names

const char* BOOK_NAMES[2] = {
  "Book 1",
  "Book 2"
};

const char* STUDENT_NAMES[2] = {
  "student 1",
  "student 2"
};


// ==========================================
// 3. BOOK STATUS DATABASE
// ==========================================

// 255 = Available
// 0   = Issued to Student 1
// 1   = Issued to Student 2

const uint8_t AVAILABLE = 255;

uint8_t issuedTo[2] = {
  AVAILABLE,
  AVAILABLE
};


// ==========================================
// 4. SYSTEM STATES
// ==========================================

enum SystemState {

  WAIT_FOR_STUDENT,
  WAIT_FOR_BOOK

};

SystemState currentState = WAIT_FOR_STUDENT;

// Currently selected student
// -1 means no student selected

int8_t selectedStudent = -1;


// ==========================================
// 5. STUDENT SESSION TIMEOUT
// ==========================================

// Timer starts when a student scans their card.

unsigned long bookWaitStartTime = 0;

// Maximum waiting time: 15 seconds

const unsigned long BOOK_SCAN_TIMEOUT = 15000UL;


// ==========================================
// 6. COMPARE RFID UID
// ==========================================

bool uidMatches(const byte expectedUID[], byte length) {

  if (rfid.uid.size != length) {
    return false;
  }

  for (byte i = 0; i < length; i++) {

    if (rfid.uid.uidByte[i] != expectedUID[i]) {
      return false;
    }

  }

  return true;
}


// ==========================================
// 7. IDENTIFY STUDENT
// ==========================================

int8_t findStudent() {

  for (byte i = 0; i < 2; i++) {

    if (uidMatches(STUDENT_UIDS[i], UID_LENGTH)) {
      return i;
    }

  }

  return -1;
}


// ==========================================
// 8. IDENTIFY BOOK
// ==========================================

int8_t findBook() {

  for (byte i = 0; i < 2; i++) {

    if (uidMatches(BOOK_UIDS[i], UID_LENGTH)) {
      return i;
    }

  }

  return -1;
}


// ==========================================
// 9. SAVE BOOK STATUS TO ESP32 MEMORY
// ==========================================

bool saveBookStatus(byte bookIndex, uint8_t borrower) {

  const char* key = (bookIndex == 0) ? "book1" : "book2";

  size_t result = preferences.putUChar(key, borrower);

  return result == 1;
}


// ==========================================
// 10. LOAD BOOK STATUS FROM MEMORY
// ==========================================

void loadBookStatus() {

  issuedTo[0] = preferences.getUChar("book1", AVAILABLE);

  issuedTo[1] = preferences.getUChar("book2", AVAILABLE);

  // Validate stored values

  for (byte i = 0; i < 2; i++) {

    if (issuedTo[i] != AVAILABLE && issuedTo[i] >= 2) {

      Serial.print("WARNING: Invalid saved status for ");
      Serial.println(BOOK_NAMES[i]);

      issuedTo[i] = AVAILABLE;

      saveBookStatus(i, AVAILABLE);

    }

  }

}


// ==========================================
// 11. DISPLAY SAVED BOOK STATUS
// ==========================================

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


// ==========================================
// 12. SCAN STUDENTS MESSAGE
// ==========================================

void scanStudents() {

  currentState = WAIT_FOR_STUDENT;

  selectedStudent = -1;

  Serial.println();
  Serial.println("================================");
  Serial.println("scan students...");
  Serial.println("================================");

}


// ==========================================
// 13. SCAN BOOK MESSAGE
// ==========================================

void scanBook() {

  currentState = WAIT_FOR_BOOK;

  Serial.println();
  Serial.print("student: ");
  Serial.print(STUDENT_NAMES[selectedStudent]);
  Serial.println("; scan book...");

}


// ==========================================
// 14. STUDENT MODE ERROR
// ==========================================

void studentError(const char* message) {

  Serial.println();
  Serial.print("ERROR: ");
  Serial.println(message);

  delay(2000);

  scanStudents();

}


// ==========================================
// 15. BOOK MODE ERROR
// ==========================================

void bookError(const char* message) {

  Serial.println();
  Serial.print("ERROR: ");
  Serial.println(message);

  delay(2000);

  // Continue with the same student
  // Original 15-second timer is not restarted.

  scanBook();

}


// ==========================================
// 16. ISSUE BOOK
// ==========================================

void issueBook(byte bookIndex) {

  Serial.println();

  // Save borrower information first

  if (saveBookStatus(bookIndex, selectedStudent)) {

    issuedTo[bookIndex] = selectedStudent;

    Serial.println("--------------------------------");

    Serial.print(BOOK_NAMES[bookIndex]);
    Serial.print(", issued to ");
    Serial.println(STUDENT_NAMES[selectedStudent]);

    Serial.println("--------------------------------");

    // Wait for 2 seconds

    delay(2000);

    // Reset system

    scanStudents();

  }

  else {

    bookError("Unable to save book status. Please try again.");

  }

}


// ==========================================
// 17. RETURN BOOK
// ==========================================

void returnBook(byte bookIndex) {

  Serial.println();

  // Save book availability first

  if (saveBookStatus(bookIndex, AVAILABLE)) {

    issuedTo[bookIndex] = AVAILABLE;

    Serial.println("--------------------------------");

    Serial.print(BOOK_NAMES[bookIndex]);
    Serial.print(", returned by ");
    Serial.println(STUDENT_NAMES[selectedStudent]);

    Serial.println("--------------------------------");

    // Wait for 2 seconds

    delay(2000);

    // Reset system

    scanStudents();

  }

  else {

    bookError("Unable to save book status. Please try again.");

  }

}


// ==========================================
// 18. SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("====================================");
  Serial.println(" RFID LIBRARY MANAGEMENT SYSTEM");
  Serial.println(" PHASE 2A - ISSUE & RETURN");
  Serial.println("====================================");

  // Start SPI communication

  SPI.begin(18, 19, 23, SS_PIN);

  // Initialize RFID reader

  rfid.PCD_Init();

  delay(4);

  // Check RFID reader

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

  if (version == 0x00 || version == 0xFF) {

    Serial.println("ERROR: RFID reader not detected!");

    while (true) {
      delay(1000);
    }

  }

  Serial.println("RFID Reader Initialized.");

  // Initialize ESP32 memory

  if (!preferences.begin("library", false)) {

    Serial.println("ERROR: Unable to open memory!");

    while (true) {
      delay(1000);
    }

  }

  // Load previously saved book status

  loadBookStatus();

  // Display library database

  displayBookStatus();

  Serial.println("SYSTEM READY!");

  // Start student scanning

  scanStudents();

}


// ==========================================
// 19. MAIN PROGRAM LOOP
// ==========================================

void loop() {


  // ========================================
  // CHECK 15-SECOND STUDENT SESSION TIMEOUT
  // ========================================

  if (currentState == WAIT_FOR_BOOK) {

    if (millis() - bookWaitStartTime >= BOOK_SCAN_TIMEOUT) {

      Serial.println();
      Serial.println("================================");
      Serial.println("TIMEOUT!");
      Serial.println("No book scanned within 15 seconds.");
      Serial.println("Student session cancelled.");
      Serial.println("================================");

      // Cancel current student session

      scanStudents();

      return;

    }

  }


  // ========================================
  // CHECK FOR NEW RFID CARD
  // ========================================

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read RFID UID

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }


  // ========================================
  // IDENTIFY SCANNED CARD
  // ========================================

  int8_t studentIndex = findStudent();

  int8_t bookIndex = findBook();


  // Display scanned UID

  Serial.print("Scanned UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      Serial.print("0");
    }

    Serial.print(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      Serial.print(":");
    }

  }

  Serial.println();


  // Stop communication with current card

  rfid.PICC_HaltA();

  rfid.PCD_StopCrypto1();


  // ========================================
  // MODE 1: WAITING FOR STUDENT
  // ========================================

  if (currentState == WAIT_FOR_STUDENT) {


    // Correct student card scanned

    if (studentIndex >= 0) {

      selectedStudent = studentIndex;

      // Start 15-second timer

      bookWaitStartTime = millis();

      Serial.println("STUDENT IDENTIFIED!");

      scanBook();

    }


    // Book scanned instead of student

    else if (bookIndex >= 0) {

      studentError("Please scan a student card, not a book.");

    }


    // Unknown RFID card

    else {

      studentError("Unregistered RFID card.");

    }

  }


  // ========================================
  // MODE 2: WAITING FOR BOOK
  // ========================================

  else if (currentState == WAIT_FOR_BOOK) {


    // Student scanned instead of book

    if (studentIndex >= 0) {

      bookError("Please scan a book card, not a student card.");

    }


    // Unknown RFID card

    else if (bookIndex < 0) {

      bookError("Unregistered RFID card. Please scan a book.");

    }


    // Valid book scanned

    else {


      // CASE 1: BOOK IS AVAILABLE

      if (issuedTo[bookIndex] == AVAILABLE) {

        issueBook(bookIndex);

      }


      // CASE 2: BOOK IS ISSUED TO SAME STUDENT

      else if (issuedTo[bookIndex] == selectedStudent) {

        returnBook(bookIndex);

      }


      // CASE 3: BOOK IS ISSUED TO ANOTHER STUDENT

      else if (issuedTo[bookIndex] < 2) {

        Serial.println();

        Serial.print("ERROR: ");
        Serial.print(BOOK_NAMES[bookIndex]);
        Serial.print(" is already issued to ");
        Serial.println(STUDENT_NAMES[issuedTo[bookIndex]]);

        delay(2000);

        // Keep the existing student session active

        scanBook();

      }

    }

  }

}
