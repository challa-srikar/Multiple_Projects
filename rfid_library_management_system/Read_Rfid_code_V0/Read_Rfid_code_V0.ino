
#include <SPI.h>
#include <MFRC522.h>

// RC522 connection pins
#define SS_PIN  5
#define RST_PIN 4

// Create RFID reader object
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {

  // Start Serial Monitor
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println(" RFID LIBRARY MANAGEMENT SYSTEM");
  Serial.println(" PHASE 1: RFID READER TEST");
  Serial.println("================================");

  // Start SPI communication
  SPI.begin(18, 19, 23, SS_PIN);

  // Initialize RFID reader
  rfid.PCD_Init();
  delay(4);

  // Check RFID reader communication
  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

  Serial.print("RC522 Version Register: 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {
    Serial.println("ERROR: RFID reader not responding!");
    Serial.println("Check the wiring and power supply.");
  }
  else {
    Serial.println("RFID Reader Initialized Successfully!");
    Serial.println();
    Serial.println("Please scan your RFID card or tag...");
  }

  Serial.println("--------------------------------");
}

void loop() {

  // Check whether a new card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read the card's UID
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println();
  Serial.println("================================");
  Serial.println(" RFID CARD DETECTED!");
  Serial.println("================================");

  // Print the UID
  Serial.print("UID: ");

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

  // Print UID length
  Serial.print("UID Length: ");
  Serial.print(rfid.uid.size);
  Serial.println(" bytes");

  // Identify card type
  MFRC522::PICC_Type piccType =
      rfid.PICC_GetType(rfid.uid.sak);

  Serial.print("Card Type: ");
  Serial.println(rfid.PICC_GetTypeName(piccType));

  Serial.println("--------------------------------");
  Serial.println("Scan completed successfully!");
  Serial.println("Remove the card before scanning again.");
  Serial.println("================================");

  // Stop communication with the current card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}
