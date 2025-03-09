#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

// Use SoftwareSerial for the AS608 sensor on pins 2 (RX) and 3 (TX)
SoftwareSerial mySerial(2, 3);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial Monitor
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("Fingerprint sensor not found. Check connections.");
    while (1);
  }
  
  Serial.println("=== Enrollment Mode ===");
  Serial.println("Enter a unique ID for this fingerprint (e.g., 101 for Employee, 201 for Voter, etc.):");
}

void loop() {
  if (Serial.available() > 0) {
    int id = Serial.parseInt();
    if (id == 0) return; // Skip if no valid input
    Serial.print("Enrolling fingerprint for ID: ");
    Serial.println(id);
    if (enrollFingerprint(id) == FINGERPRINT_OK) {
      simulateDigitalSlip();
    }
    Serial.println("Enrollment complete. Enter next unique ID if needed:");
  }
}

uint8_t enrollFingerprint(int id) {
  int p = 0;
  Serial.println("Place finger on sensor...");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) continue;
    Serial.println("Error capturing image. Try again.");
  }
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting image.");
    return p;
  }
  
  Serial.println("Remove finger.");
  delay(2000);
  
  Serial.println("Place the same finger again...");
  while ((p = finger.getImage()) != FINGERPRINT_OK) {
    if (p == FINGERPRINT_NOFINGER) continue;
    Serial.println("Error capturing image. Try again.");
  }
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error converting image.");
    return p;
  }
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error.");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match.");
    return p;
  }
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Fingerprint stored successfully!");
  } else {
    Serial.println("Error storing fingerprint.");
  }
  
  return p;
}

void simulateDigitalSlip() {
  Serial.println("=== Digital Slip Simulation ===");
  Serial.println("Select your preferred voting slot (For this prototype, all choose Slot 1).");
  Serial.println("====================================");
  delay(3000);
}