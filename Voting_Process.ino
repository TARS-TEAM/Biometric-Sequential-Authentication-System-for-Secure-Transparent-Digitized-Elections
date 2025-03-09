#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <RTClib.h>

// RTC DS1307 (I2C: A4 = SDA, A5 = SCL)
RTC_DS1307 rtc;

// AS608 Fingerprint Sensor 1 for entry verification (Employee & Voter Index)
SoftwareSerial fingerSerial1(2, 3);
Adafruit_Fingerprint finger1 = Adafruit_Fingerprint(&fingerSerial1);

// AS608 Fingerprint Sensor 2 for vote confirmation (Voter Thumb)
SoftwareSerial fingerSerial2(4, 5);
Adafruit_Fingerprint finger2 = Adafruit_Fingerprint(&fingerSerial2);

// LED pins for entry verification
const int ledGreen  = 6;  // Success indicator
const int ledYellow = 7;  // Employee mismatch indicator
const int ledRed    = 8;  // Error/cheating indicator

// LED pins for vote confirmation
const int voteGreen = 10; // Vote confirmed indicator
const int voteRed   = 11; // Vote confirmation failure

// Party vote button pins (using internal pull-up)
const int party1ButtonPin = 12;
const int party2ButtonPin = 13;
const int party3ButtonPin = A0;
const int party4ButtonPin = A1;

// Shift Timing (24-hour clock)
// Shift 1: 9:00-11:00 AM; Shift 2: 11:00-1:00 PM; Shift 3: 1:00-3:00 PM; Shift 4: 3:00-4:00 PM
#define SHIFT1_START 9
#define SHIFT1_END   11
#define SHIFT2_START 11
#define SHIFT2_END   13
#define SHIFT3_START 13
#define SHIFT3_END   15
#define SHIFT4_START 15
#define SHIFT4_END   16

// Global voter flags (simulate voters with IDs: 201, 202, 203, 204, 205, 206, 207)
bool voted201 = false; // Voter 1
bool voted202 = false; // Voter 2
bool voted203 = false; // Voter 3
bool voted204 = false; // Voter 4
bool voted205 = false; // Voter 5
bool voted206 = false; // Voter 6
bool voted207 = false; // Voter 7

// Function prototypes
uint8_t getFingerprintID(Adafruit_Fingerprint &finger);
int getCurrentShift();
bool isAllowedVoter(int currentShift, int voterID);
void markVoted(int voterID);
void simulateDigitalSlip(int currentShift);
void simulateReminderMessages();
void entryVerification();
void voteVerification(int selectedParty);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  pinMode(ledGreen, OUTPUT);
  pinMode(ledYellow, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(voteGreen, OUTPUT);
  pinMode(voteRed, OUTPUT);
  
  pinMode(party1ButtonPin, INPUT_PULLUP);
  pinMode(party2ButtonPin, INPUT_PULLUP);
  pinMode(party3ButtonPin, INPUT_PULLUP);
  pinMode(party4ButtonPin, INPUT_PULLUP);
  
  if (!rtc.begin()) {
    Serial.println("RTC not found.");
    while (1);
  }
  if (!rtc.isrunning()) {
    Serial.println("RTC not running; setting time.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  fingerSerial1.begin(57600);
  finger1.begin(57600);
  if (finger1.verifyPassword()) {
    Serial.println("Fingerprint sensor 1 (Entry) found!");
  } else {
    Serial.println("Fingerprint sensor 1 not found.");
    while (1);
  }
  
  fingerSerial2.begin(57600);
  finger2.begin(57600);
  if (finger2.verifyPassword()) {
    Serial.println("Fingerprint sensor 2 (Vote Confirmation) found!");
  } else {
    Serial.println("Fingerprint sensor 2 not found.");
    while (1);
  }
  
  Serial.println("System Initialized. Ready for voting process.");
}

void loop() {
  simulateReminderMessages();
  
  DateTime now = rtc.now();
  if (now.hour() >= 16) {
    Serial.println("Election period is over. Final Results:");
    Serial.print("Party 1 Votes: "); Serial.println(party1Votes);
    Serial.print("Party 2 Votes: "); Serial.println(party2Votes);
    Serial.print("Party 3 Votes: "); Serial.println(party3Votes);
    Serial.print("Party 4 Votes: "); Serial.println(party4Votes);
    int maxVotes = party1Votes;
    int winner = 1;
    if (party2Votes > maxVotes) { maxVotes = party2Votes; winner = 2; }
    if (party3Votes > maxVotes) { maxVotes = party3Votes; winner = 3; }
    if (party4Votes > maxVotes) { maxVotes = party4Votes; winner = 4; }
    Serial.print("Winner is Party "); Serial.print(winner);
    Serial.print(" with "); Serial.print(maxVotes);
    Serial.println(" votes.");
    Serial.println("Eligible voters receive government incentives (tax waivers/subsidies).");
    while (1);
  }
  
  int currentShift = getCurrentShift();
  if (currentShift == 0) {
    Serial.println("Not within any voting slot period. Waiting...");
    delay(5000);
    return;
  }
  Serial.print("Current Shift: "); Serial.println(currentShift);
  
  simulateDigitalSlip(currentShift);
  
  entryVerification();
  
  Serial.println("Waiting for party selection... (Press a party button)");
  int selectedParty = 0;
  while (selectedParty == 0) {
    if (digitalRead(party1ButtonPin) == LOW) { selectedParty = 1; }
    else if (digitalRead(party2ButtonPin) == LOW) { selectedParty = 2; }
    else if (digitalRead(party3ButtonPin) == LOW) { selectedParty = 3; }
    else if (digitalRead(party4ButtonPin) == LOW) { selectedParty = 4; }
  }
  delay(200);
  Serial.print("Party "); Serial.print(selectedParty); Serial.println(" selected.");
  
  voteVerification(selectedParty);
  
  Serial.println("Vote recorded. Thank you for voting.");
  delay(5000);
}

uint8_t getFingerprintID(Adafruit_Fingerprint &finger) {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return p;
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return p;
  
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    Serial.println("Fingerprint not found.");
    return p;
  }
  
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence: ");
  Serial.println(finger.confidence);
  return finger.fingerID;
}

int getCurrentShift() {
  DateTime now = rtc.now();
  int hour = now.hour();
  if (hour >= SHIFT1_START && hour < SHIFT1_END) return 1;
  else if (hour >= SHIFT2_START && hour < SHIFT2_END) return 2;
  else if (hour >= SHIFT3_START && hour < SHIFT3_END) return 3;
  else if (hour >= SHIFT4_START && hour < SHIFT4_END) return 4;
  else return 0;
}

bool isAllowedVoter(int currentShift, int voterID) {
  if (currentShift == 1) return (voterID == 201);
  else if (currentShift == 2) return (voterID == 202 || voterID == 203 || voterID == 206);
  else if (currentShift == 3) return (voterID == 204);
  else if (currentShift == 4) return (voterID == 204 || voterID == 205);
  return false;
}

void markVoted(int voterID) {
  switch(voterID) {
    case 201: voted201 = true; break;
    case 202: voted202 = true; break;
    case 203: voted203 = true; break;
    case 204: voted204 = true; break;
    case 205: voted205 = true; break;
    case 206: voted206 = true; break;
    case 207: voted207 = true; break;
  }
}

void simulateDigitalSlip(int currentShift) {
  Serial.println("=== Digital Slip / Reminder ===");
  Serial.print("Your assigned voting slot is: ");
  switch (currentShift) {
    case 1: Serial.println("9:00 AM - 11:00 AM"); break;
    case 2: Serial.println("11:00 AM - 1:00 PM"); break;
    case 3: Serial.println("1:00 PM - 3:00 PM"); break;
    case 4: Serial.println("3:00 PM - 4:00 PM"); break;
    default: Serial.println("Slot not assigned."); break;
  }
  Serial.println("If you have not voted yet, please come to vote in your current slot.");
  Serial.println("====================================");
  delay(3000);
}

void simulateReminderMessages() {
  Serial.println("=== Reminder Messages ===");
  if (!voted201) Serial.println("Reminder: Voter 1 (ID 201), please vote.");
  if (!voted202) Serial.println("Reminder: Voter 2 (ID 202), please vote.");
  if (!voted203) Serial.println("Reminder: Voter 3 (ID 203), please vote.");
  if (!voted204) Serial.println("Reminder: Voter 4 (ID 204), please vote.");
  if (!voted205) Serial.println("Reminder: Voter 5 (ID 205), please vote.");
  if (!voted206) Serial.println("Reminder: Voter 6 (ID 206), please vote.");
  if (!voted207) Serial.println("Reminder: Voter 7 (ID 207), please vote.");
  Serial.println("=========================");
  delay(2000);
}

void entryVerification() {
  int currentShift = getCurrentShift();
  if (currentShift == 0) {
    Serial.println("Entry verification attempted outside shift hours.");
    return;
  }
  
  int expectedEmployee = 0;
  int expectedVoter = -1;
  if (currentShift == 1) { expectedEmployee = 101; expectedVoter = 201; }
  else if (currentShift == 2) { expectedEmployee = 102; expectedVoter = 204; }
  else if (currentShift == 3) { expectedEmployee = 103; expectedVoter = 204; }
  else if (currentShift == 4) { expectedEmployee = 104; } // Allowed voters: 202 or 205
  
  Serial.println("Place EMPLOYEE fingerprint (Thumb) on sensor 1...");
  uint8_t empID = getFingerprintID(finger1);
  delay(1000);
  if (empID != expectedEmployee) {
    Serial.println("Employee fingerprint mismatch. Unauthorized or wrong shift.");
    digitalWrite(ledYellow, HIGH);
    delay(2000);
    digitalWrite(ledYellow, LOW);
    return;
  }
  Serial.println("Employee verified.");
  
  Serial.println("Place VOTER fingerprint (Index) on sensor 1...");
  uint8_t voterID = getFingerprintID(finger1);
  delay(1000);
  
  if (currentShift == 4) {
    if (!(voterID == 202 || voterID == 205)) {
      Serial.println("Voter fingerprint mismatch. Not allocated for final slot.");
      digitalWrite(ledRed, HIGH);
      delay(2000);
      digitalWrite(ledRed, LOW);
      return;
    }
  } else {
    if (voterID != expectedVoter) {
      Serial.println("Voter fingerprint mismatch or illegal attempt.");
      digitalWrite(ledRed, HIGH);
      delay(2000);
      digitalWrite(ledRed, LOW);
      return;
    }
  }
  Serial.println("Voter verified. Entry granted.");
  digitalWrite(ledGreen, HIGH);
  delay(2000);
  digitalWrite(ledGreen, LOW);
  
  markVoted(voterID);
}

void voteVerification(int selectedParty) {
  Serial.println("Place VOTER fingerprint (Thumb) on sensor 2 for vote confirmation...");
  uint8_t voterThumbID = getFingerprintID(finger2);
  delay(1000);
  
  int currentShift = getCurrentShift();
  
  if (!isAllowedVoter(currentShift, voterThumbID)) {
    Serial.println("Vote confirmation failed. Fingerprint mismatch in current shift.");
    digitalWrite(voteRed, HIGH);
    delay(2000);
    digitalWrite(voteRed, LOW);
    return;
  }
  
  if (currentShift == 3) { 
    if (voterThumbID == 203) { 
      Serial.println("Cheating detected in Shift 3! Voter 3 attempted to vote as Voter 4.");
      digitalWrite(voteRed, HIGH);
      delay(2000);
      digitalWrite(voteRed, LOW);
      return;
    }
  }
  if (currentShift == 2) {
    if (voterThumbID == 206 && voted206) {
      Serial.println("Cheating detected in Shift 2! Voter 6 attempting to vote again.");
      digitalWrite(voteRed, HIGH);
      delay(2000);
      digitalWrite(voteRed, LOW);
      return;
    }
  }
  if (currentShift == 4) {
    if (voterThumbID == 206) {
      Serial.println("Cheating detected in Shift 4! Voter 6 not allowed in final slot.");
      digitalWrite(voteRed, HIGH);
      delay(2000);
      digitalWrite(voteRed, LOW);
      return;
    }
  }
  
  Serial.println("Vote confirmed successfully.");
  digitalWrite(voteGreen, HIGH);
  delay(2000);
  digitalWrite(voteGreen, LOW);
  
  switch(selectedParty) {
    case 1: party1Votes++; break;
    case 2: party2Votes++; break;
    case 3: party3Votes++; break;
    case 4: party4Votes++; break;
  }
}