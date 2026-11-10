
#include <SPI.h>
#include <MFRC522.h>

// =================================
// RFID CONNECTION PINS
// =================================

#define SS_PIN  5
#define RST_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// =================================
// REGISTERED RFID UIDs
// =================================

// BOOKS
const byte BOOK1_UID[] = {0xFD, 0x32, 0x8F, 0xD0};
const byte BOOK2_UID[] = {0x77, 0x18, 0x75, 0x06};

// STUDENTS
const byte STUDENT1_UID[] = {0xB1, 0x83, 0x19, 0x66};
const byte STUDENT2_UID[] = {0x35, 0x46, 0xF6, 0x05};

// =================================
// FUNCTION: CHECK RFID UID
// =================================

bool compareUID(const byte registeredUID[], byte length) {

  if (rfid.uid.size != length) {
    return false;
  }

  for (byte i = 0; i < length; i++) {

    if (rfid.uid.uidByte[i] != registeredUID[i]) {
      return false;
    }

  }

  return true;
}

// =================================
// SETUP
// =================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("==================================");
  Serial.println(" RFID LIBRARY MANAGEMENT SYSTEM");
  Serial.println(" UID REGISTRATION TEST");
  Serial.println("==================================");

  SPI.begin(18, 19, 23, SS_PIN);

  rfid.PCD_Init();
  delay(4);

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

  Serial.print("RC522 Version: 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {

    Serial.println("ERROR: RFID reader not detected!");
    while (true) {
      delay(1000);
    }

  }

  Serial.println("RFID Reader Ready!");
  Serial.println();
  Serial.println("Scan any registered RFID card or tag.");
  Serial.println("----------------------------------");
}

// =================================
// LOOP
// =================================

void loop() {

  // Check for a new RFID card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card UID
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("==================================");
  Serial.println("       RFID CARD DETECTED");
  Serial.println("==================================");

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
  Serial.println("----------------------------------");

  // =================================
  // IDENTIFY BOOKS
  // =================================

  if (compareUID(BOOK1_UID, sizeof(BOOK1_UID))) {

    Serial.println("IDENTITY : BOOK 1");
    Serial.println("CATEGORY : BOOK");
    Serial.println("STATUS   : REGISTERED");

  }

  else if (compareUID(BOOK2_UID, sizeof(BOOK2_UID))) {

    Serial.println("IDENTITY : BOOK 2");
    Serial.println("CATEGORY : BOOK");
    Serial.println("STATUS   : REGISTERED");

  }

  // =================================
  // IDENTIFY STUDENTS
  // =================================

  else if (compareUID(STUDENT1_UID, sizeof(STUDENT1_UID))) {

    Serial.println("IDENTITY : STUDENT 1");
    Serial.println("CATEGORY : STUDENT");
    Serial.println("STATUS   : REGISTERED");

  }

  else if (compareUID(STUDENT2_UID, sizeof(STUDENT2_UID))) {

    Serial.println("IDENTITY : STUDENT 2");
    Serial.println("CATEGORY : STUDENT");
    Serial.println("STATUS   : REGISTERED");

  }

  // =================================
  // UNKNOWN RFID CARD
  // =================================

  else {

    Serial.println("IDENTITY : UNKNOWN");
    Serial.println("CATEGORY : NOT REGISTERED");
    Serial.println("STATUS   : ACCESS DENIED");

  }

  Serial.println("==================================");

  // Stop communication with current card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}
