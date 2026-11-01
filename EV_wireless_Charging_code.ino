#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Configuration
#define IR_sensor 2
#define RelayPin 4

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

// Charging Variables
unsigned long chargeStart = 0;
bool charging = false;
int chargePercent = 0;

const unsigned long updateInterval = 60000UL; 
// Increase 1% every 60 seconds

// Authorized RFID UIDs
byte allowedUID1[] = {0xD7, 0xAC, 0xA6, 0x05};
byte allowedUID2[] = {0x72, 0xC0, 0x56, 0x5C};

// Check RFID UID
bool checkUID(byte *tag, byte *allowed) {
  for (byte i = 0; i < 4; i++) {
    if (tag[i] != allowed[i]) {
      return false;
    }
  }
  return true;
}

// Display waiting message
void showWaiting() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Waiting for");
  lcd.setCursor(0, 1);
  lcd.print("Vehicle...");
}

// Check vehicle presence
bool vehiclePresent() {
  return digitalRead(IR_sensor) == LOW;
}

// Stop charging
void stopCharging() {
  charging = false;
  digitalWrite(RelayPin, LOW);
}

// Setup
void setup() {

  lcd.init();
  lcd.backlight();

  SPI.begin();
  rfid.PCD_Init();

  pinMode(IR_sensor, INPUT);
  pinMode(RelayPin, OUTPUT);

  digitalWrite(RelayPin, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wireless Charging");
  delay(1500);

  showWaiting();
}

// Main Loop
void loop() {

  // 1. Vehicle Detection
  if (!vehiclePresent()) {
    stopCharging();
    showWaiting();
    delay(500);
    return;
  }

  // 2. Ask for RFID Authentication
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vehicle Detected");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID...");

  // Wait for RFID scan
  while (!rfid.PICC_IsNewCardPresent() ||
         !rfid.PICC_ReadCardSerial()) {

    if (!vehiclePresent()) {
      showWaiting();
      return;
    }
  }

  // 3. Verify RFID
  bool authorized = false;

  if (rfid.uid.size == 4) {
    authorized =
      checkUID(rfid.uid.uidByte, allowedUID1) ||
      checkUID(rfid.uid.uidByte, allowedUID2);
  }

  // Finish RFID communication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  if (!authorized) {

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unauthorized");
    lcd.setCursor(0, 1);
    lcd.print("Vehicle!");

    delay(1500);
    return;
  }

  // 4. RFID Verified
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Verified!");

  delay(1000);

  if (!vehiclePresent()) {
    showWaiting();
    return;
  }

  // 5. Coil Alignment
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Please Check");
  lcd.setCursor(0, 1);
  lcd.print("Coil Alignment");

  for (int i = 0; i < 5; i++) {

    delay(1000);

    if (!vehiclePresent()) {
      showWaiting();
      return;
    }
  }

  // 6. Five Second Countdown
  for (int i = 5; i >= 1; i--) {

    if (!vehiclePresent()) {
      showWaiting();
      return;
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Starting...");
    lcd.setCursor(0, 1);
    lcd.print("In ");
    lcd.print(i);
    lcd.print(" seconds");

    delay(1000);
  }

  // 7. Start Charging
  chargePercent = 0;
  charging = true;

  chargeStart = millis();

  digitalWrite(RelayPin, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Charging Started");

  delay(1000);

  // 8. Charging Process
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wireless Charging");

  while (charging) {

    // Check vehicle presence
    if (!vehiclePresent()) {

      stopCharging();
      showWaiting();
      return;
    }

    // Calculate charging time
    unsigned long currentTime = millis();

    if (currentTime - chargeStart >= updateInterval) {

      chargeStart += updateInterval;

      if (chargePercent < 100) {
        chargePercent++;
      }
    }

    // Update LCD
    lcd.setCursor(0, 1);
    lcd.print("Charge: ");
    lcd.print(chargePercent);
    lcd.print("%     ");

    // Stop at 100%
    if (chargePercent >= 100) {

      stopCharging();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Charging Done!");
      lcd.setCursor(0, 1);
      lcd.print("100% Complete");

      delay(2000);
      showWaiting();
      return;
    }

    delay(200);
  }
}