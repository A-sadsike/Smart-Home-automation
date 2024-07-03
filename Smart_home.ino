#define BLYNK_TEMPLATE_ID "TMPL6MahsIiNq"
#define BLYNK_TEMPLATE_NAME "intru"

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#define SS_PIN 21   // Set the SS pin for RFID RC522
#define RST_PIN 22  // Set the RST pin for RFID RC522
#define LDR_PIN A0  // Set the pin for the LDR
#define LED_PIN 2   // Use the built-in LED pin for ESP32

MFRC522 mfrc522(SS_PIN, RST_PIN);

char auth[] = "upsSHWJj8-EaLv9uDvfdEVrEJ__Me2Q0";
char ssid[] = "Iqbal-3";
char password[] = "";

int ldrThreshold = 150;
bool isDay = false;
bool manualControl = false; // New variable for manual control

struct RFIDCard {
  String cardNumber;
  int tapCount;
};

const int maxCards = 10;  // Maximum number of RFID cards to handle
RFIDCard cards[maxCards]; // Array to store RFID cards and tap counts
int numCards = 0;         // Number of detected RFID cards

BLYNK_WRITE(V1) {
  // This function is called when the LDR threshold slider is adjusted in the Blynk app
  ldrThreshold = param.asInt();
}

BLYNK_WRITE(V3) {
  // This function is called when the Blynk switch is turned on or off for manual control
  manualControl = param.asInt();
  digitalWrite(LED_PIN, manualControl ? HIGH : LOW); // Set LED state based on the switch state
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(auth, ssid, password);
  SPI.begin();
  mfrc522.PCD_Init();  // Initialize the RFID module
  delay(2000);         // Allow some time for the RFID module to initialize
  pinMode(LED_PIN, OUTPUT);
  Blynk.syncVirtual(V1); // Sync LDR threshold with the app
}

void loop() {
  Blynk.run();
  mfrc522Loop();
  ldrControl();
}

void mfrc522Loop() {
  Serial.println("Scanning for RFID card...");
  delay(1000); // Add a delay to stabilize RFID scanning
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String cardNumber = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardNumber += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
      cardNumber += String(mfrc522.uid.uidByte[i], HEX);
    }
    handleRFIDCard(cardNumber);
    mfrc522.PICC_HaltA();
  }
}

void handleRFIDCard(String cardNumber) {
  int cardIndex = getCardIndex(cardNumber);
  Serial.print("Card Index: ");
  Serial.println(cardIndex);

  if (cardIndex == -1) {
    // New card detected, add it to the array
    if (numCards < maxCards) {
      cards[numCards].cardNumber = cardNumber;
      cards[numCards].tapCount = 0;
      numCards++;
      Serial.println("New Card Added");
    } else {
      Serial.println("Max Cards Reached");
    }
  } else {
    // Existing card detected, increment the tap count
    cards[cardIndex].tapCount++;
    Serial.println("Tap Count Incremented");
  }

  // Send RFID card information to Blynk
  Blynk.virtualWrite(V1, cardNumber + " Taps: " + String(cards[cardIndex].tapCount));
}


int getCardIndex(String cardNumber) {
  for (int i = 0; i < numCards; i++) {
    if (cards[i].cardNumber.equals(cardNumber)) {
      return i;
    }
  }
  return -1;
}

void ldrControl() {
  int ldrValue = analogRead(LDR_PIN);
  isDay = (ldrValue > ldrThreshold);

  // Update LED based on RFID tap counts and manual control
  bool shouldTurnOn = false;

  for (int i = 0; i < numCards; i++) {
    if (cards[i].tapCount % 2 != 0) {
      shouldTurnOn = true;
      break;
    }
  }

  if (manualControl) {
    digitalWrite(LED_PIN, HIGH); // Turn on LED based on manual control
  } else {
    if (!shouldTurnOn) {
      digitalWrite(LED_PIN, LOW); // Turn off LED
    } else {
      digitalWrite(LED_PIN, isDay ? LOW : HIGH); // Turn on LED based on LDR logic
    }
  }

  // Send day/night status to Blynk
  Blynk.virtualWrite(V2, isDay ? "Day" : "Night");
}
