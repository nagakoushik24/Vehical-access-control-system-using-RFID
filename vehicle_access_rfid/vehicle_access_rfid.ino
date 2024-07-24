#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// Pin Definitions
#define SS_PIN 9
#define RST_PIN 8
#define ENTRANCE_SENSOR_PIN 2
#define EXIT_SENSOR_PIN 3
#define SERVO_PIN 4
#define GREEN_LED_PIN 5
#define RED_LED_PIN 6

// RFID Module
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Servo Motor
Servo gateServo;

// Create a dictionary to store authorized UIDs and their respective names
struct AuthorizedCard {
  String uid;
  String name;
};

AuthorizedCard authorizedCards[] = {
  { "0E43257D", "Alice" },
  { "35227", "Bob" }
};
const int numAuthorizedCards = sizeof(authorizedCards) / sizeof(AuthorizedCard);

// Variables
bool vehicleDetected = false;
bool gateOpen = false;
unsigned long lastEntranceSensorTriggerTime = 0;
unsigned long lastExitSensorTriggerTime = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(ENTRANCE_SENSOR_PIN, INPUT);
  pinMode(EXIT_SENSOR_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(90); // Initial position (gate closed)

  Serial.println("System Initialized. Waiting for vehicle...");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check the entrance sensor with debounce
  if (digitalRead(ENTRANCE_SENSOR_PIN) == HIGH && digitalRead(EXIT_SENSOR_PIN) == LOW) {
    lastEntranceSensorTriggerTime = currentMillis;
    if (vehicleDetected == false) {
      vehicleDetected = true;
      closeGate()
      Serial.println("Vehicle detected at the entrance");
    }
  }
  
  int scanResult = scanRFID();
  if (scanResult == 1) {
    openGate();
  } else if (scanResult == 0) {
    denyAccess();
  }
  // Check the exit sensor with debounce
  if (digitalRead(EXIT_SENSOR_PIN) == HIGH && digitalRead(ENTRANCE_SENSOR_PIN) == LOW) {
    lastExitSensorTriggerTime = currentMillis;
    if (vehicleDetected == true) {
      vehicleDetected = false;
      Serial.println("Vehicle detected at the exit");
      closeGate(); // Close gate when vehicle reaches exit sensor
    }
  }
}

int scanRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return -1; // No card present
  }
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  Serial.print("Card UID: ");
  Serial.println(uid);
  mfrc522.PICC_HaltA();

  return isAuthorized(uid) ? 1 : 0;
}

bool isAuthorized(String uid) {
  for (int i = 0; i < numAuthorizedCards; i++) {
    if (uid == authorizedCards[i].uid) {
      Serial.print("Welcome ");
      Serial.println(authorizedCards[i].name);
      return true;
    }
  }
  return false;
}

void openGate() {
  Serial.println("Access granted. Opening gate...");
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  gateServo.write(0); // Open gate
  gateOpen = true;

  delay(2000); // Keep the red LED on for 2 seconds

  digitalWrite(GREEN_LED_PIN, LOW);
}

void denyAccess() {
  Serial.println("Access denied.");
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  delay(2000); // Keep the red LED on for 2 seconds
  digitalWrite(RED_LED_PIN, LOW);
}

void closeGate() {
  if (gateOpen) {
    Serial.println("Closing gate...");
    gateServo.write(90); // Close gate
    gateOpen = false;
  }
}