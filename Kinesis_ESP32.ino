/**
 * kinesis_esp32_S3_CORRIGE.ino
 * Firmware Kinesis — Adapté et sécurisé pour ESP32-S3-DevKitC-1-N8R8
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>

// ==========================
// WIFI
// ==========================
const char* ssid = "Etudiants_ISM";
const char* password = "Etudiant123&";

// ==========================
// API
// ==========================
const String rfidAccessURL   = "https://kinesis.alwaysdata.net/Kinnesis/rfid_access.php";
const String rfidEnrollURL   = "https://kinesis.alwaysdata.net/Kinnesis/rfid_enroll.php";
const String fingerAccessURL = "https://kinesis.alwaysdata.net/Kinnesis/finger_access.php";
const String fingerEnrollURL = "https://kinesis.alwaysdata.net/Kinnesis/finger_enroll.php";
const String stateURL        = "https://kinesis.alwaysdata.net/Kinnesis/get_device_state.php";
const String heartbeatURL    = "https://kinesis.alwaysdata.net/Kinnesis/device_heartbeat.php";

const String numero_salle = "101";

// =========================================================================
// BROCHAGE SÛR POUR ESP32-S3 N8R8 (Modifié pour éviter les broches fantômes)
// =========================================================================

// RFID RC522 (PINS SPI standardisés pour ESP32-S3)
#define SS_PIN  10  
#define RST_PIN  9  
MFRC522 rfid(SS_PIN, RST_PIN);

// CAPTEUR EMPREINTE (Bascule sur Serial1 — PINS 14 & 15 libres de PSRAM)
#define FP_RX 14    
#define FP_TX 15    
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1); 
bool fingerSensorOK = false;

// LED RGB (PINS existants et sécurisés sur S3)
#define RED_PIN   4  
#define GREEN_PIN 5  
#define BLUE_PIN  6  

// BUZZER
#define BUZZER_PIN 7 

// ==========================
// VARIABLES GLOBALES
// ==========================
unsigned long lastHeartbeat     = 0;
const long    heartbeatInterval = 10000;

String currentMode   = "access";
int    serrureEtat   = 0;
bool   serverErrorActive = false;

unsigned long lastBlink = 0;
bool blinkState = false;

// ──────────────────────────────────────────────
// LED & SIGNAUX
// ──────────────────────────────────────────────

void setColor(bool red, bool green, bool blue) {
  digitalWrite(RED_PIN,   red);
  digitalWrite(GREEN_PIN, green);
  digitalWrite(BLUE_PIN,  blue);
}

void updateLedState() {
  if (millis() - lastBlink < 500) return;
  lastBlink  = millis();
  blinkState = !blinkState;

  if (WiFi.status() != WL_CONNECTED || serverErrorActive) {
    if (blinkState) setColor(HIGH, LOW, LOW);
    else            setColor(LOW,  LOW, LOW);
    return;
  }

  if (currentMode == "enroll") {
    if (blinkState) setColor(LOW, LOW, HIGH);
    else            setColor(LOW, LOW, LOW);
    return;
  }

  if (currentMode == "finger_enroll") {
    if (blinkState) setColor(HIGH, LOW, HIGH); 
    else            setColor(LOW,  LOW, LOW);
    return;
  }

  if (serrureEtat == 1) {
    setColor(LOW, HIGH, LOW);
    return;
  }

  setColor(HIGH, LOW, LOW);
}

void accessGranted() {
  setColor(LOW, HIGH, LOW);
  tone(BUZZER_PIN, 2000, 300);
  delay(1000);
}

void accessDenied() {
  for (int i = 0; i < 2; i++) {
    setColor(HIGH, LOW, LOW);
    tone(BUZZER_PIN, 800, 150);
    delay(200);
    setColor(LOW, LOW, LOW);
    delay(150);
  }
}

void enrollSuccess() {
  setColor(LOW, HIGH, LOW);
  tone(BUZZER_PIN, 1500, 100); delay(150);
  tone(BUZZER_PIN, 2000, 100); delay(150);
  tone(BUZZER_PIN, 2500, 200); delay(400);
}

// ──────────────────────────────────────────────
// LOGIQUE API SERVEUR
// ──────────────────────────────────────────────

void getDeviceState() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(stateURL + "?numero=" + numero_salle);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("STATE: " + payload);

    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);

    if (doc["success"].as<bool>()) {
      serrureEtat       = doc["etat_serrure"].as<int>();
      currentMode       = doc["mode"].as<String>();
      serverErrorActive = false;
    } else {
      serverErrorActive = true;
    }
  } else {
    serverErrorActive = true;
  }
  http.end();
}

void handleRFIDAccess(const String& uid) {
  HTTPClient http;
  String url = rfidAccessURL + "?uid=" + uid + "&numero=" + numero_salle;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("RFID ACCESS: " + payload);
    if (payload.indexOf("\"success\":true") >= 0) {
      accessGranted();
      serverErrorActive = false;
    } else {
      accessDenied();
      serverErrorActive = false;
    }
  } else {
    serverErrorActive = true;
  }
  http.end();
}

void handleRFIDEnroll(const String& uid) {
  HTTPClient http;
  String url = rfidEnrollURL + "?uid=" + uid;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("RFID ENROLL: " + payload);
    if (payload.indexOf("\"success\":true") >= 0) {
      enrollSuccess();
      serverErrorActive = false;
    } else {
      accessDenied();
      serverErrorActive = false;
    }
  } else {
    serverErrorActive = true;
  }
  http.end();
}

int readFingerprintNonBlocking() {
  if (!fingerSensorOK) return -1;

  int p = finger.getImage();
  if (p == FINGERPRINT_NOFINGER) return -1;
  if (p != FINGERPRINT_OK) return -2;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -2;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.print("Empreinte trouvée, ID: ");
    Serial.println(finger.fingerID);
    return finger.fingerID;
  }
  return -2;
}

void handleFingerAccess(int fingerprintId) {
  HTTPClient http;
  String url = fingerAccessURL + "?fingerprint_id=" + String(fingerprintId) + "&numero=" + numero_salle;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("FINGER ACCESS: " + payload);
    if (payload.indexOf("\"success\":true") >= 0) {
      accessGranted();
      serverErrorActive = false;
    } else {
      accessDenied();
      serverErrorActive = false;
    }
  } else {
    serverErrorActive = true;
  }
  http.end();
}

void handleFingerEnroll() {
  if (!fingerSensorOK) return;

  Serial.println("\n--- MODE ENROLEMENT EMPREINTE ---");
  uint16_t newId = 0;
  for (uint16_t slot = 1; slot <= 127; slot++) {
    int p = finger.loadModel(slot);
    if (p != FINGERPRINT_OK) {
      newId = slot;
      break;
    }
  }
  if (newId == 0) { accessDenied(); return; }
  
  Serial.println("Posez le doigt sur le capteur...");
  int p = -1;
  while (p != FINGERPRINT_OK) {
    updateLedState();
    p = finger.getImage();
    delay(50);
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) return;

  Serial.println("Retirez le doigt...");
  tone(BUZZER_PIN, 1500, 100);
  while (p != FINGERPRINT_NOFINGER) {
    updateLedState();
    p = finger.getImage();
    delay(100);
  }
  
  p = -1;
  Serial.println("Reposez le MEME doigt...");
  while (p != FINGERPRINT_OK) {
    updateLedState();
    p = finger.getImage();
    delay(50);
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) return;

  p = finger.createModel();
  if (p != FINGERPRINT_OK) { accessDenied(); return; }

  p = finger.storeModel(newId);
  if (p == FINGERPRINT_OK) {
    Serial.println("Enregistré localement ! Envoi au serveur...");
  } else { return; }

  HTTPClient http;
  String url = fingerEnrollURL + "?fingerprint_id=" + String(newId) + "&numero=" + numero_salle;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("REPONSE SERVEUR: " + payload);
    if (payload.indexOf("\"success\":true") >= 0) {
      enrollSuccess();
      serverErrorActive = false;
    } else {
      finger.deleteModel(newId);
      accessDenied();
    }
  } else {
    serverErrorActive = true;
    finger.deleteModel(newId);
  }
  http.end();
  while (finger.getImage() != FINGERPRINT_NOFINGER) delay(100);
}

// ──────────────────────────────────────────────
// SETUP
// ──────────────────────────────────────────────

void setup() {
  // 1. Initialisation immédiate de l'USB Série
  Serial.begin(115200);

  // 2. Pause indispensable pour l'USB CDC natif (3 secondes)
  for (int i = 0; i < 3; i++) {
    delay(1000);
  }

  Serial.println("\n===============================================");
  Serial.println("   DEMARRAGE SYSTEME KINESIS - ESP32-S3 N8R8    ");
  Serial.println("===============================================");

  pinMode(RED_PIN,   OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN,  OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialisation du bus SPI natif pour le RC522 (S3 : SCK=12, MISO=13, MOSI=11, SS=10)
  SPI.begin(12, 13, 11, SS_PIN);
  rfid.PCD_Init();
  Serial.println("-> RFID RC522 configuré.");

  // Configuration de Serial1 pour le capteur d'empreinte (PINS 14 et 15)
  Serial1.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  if (finger.verifyPassword()) {
    fingerSensorOK = true;
    Serial.println("-> Capteur empreinte détecté ✓");
  } else {
    fingerSensorOK = false;
    Serial.println("-> Capteur empreinte non détecté (A vide).");
  }

  // Connexion WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connexion au WiFi");
  
  // Timeout de sécurité pour éviter de bloquer indéfiniment si pas de réseau pendant le test
  int timeout_count = 0;
  while (WiFi.status() != WL_CONNECTED && timeout_count < 20) {
    setColor(HIGH, LOW, LOW); delay(250);
    setColor(LOW,  LOW, LOW); delay(250);
    Serial.print(".");
    timeout_count++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\n-> WiFi connecté ✓ IP: " + WiFi.localIP().toString());
    getDeviceState();
  } else {
    Serial.println("\n-> [ATTENTION] Connexion WiFi en arrière-plan...");
  }
}

// ──────────────────────────────────────────────
// LOOP
// ──────────────────────────────────────────────

void loop() {
  updateLedState();

  if (millis() - lastHeartbeat > heartbeatInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(heartbeatURL + "?numero=" + numero_salle);
      int code = http.GET();
      Serial.println("Heartbeat réseau : " + String(code));
      http.end();
      getDeviceState();
    } else {
      Serial.println("Heartbeat ignoré (WiFi non connecté)");
    }
    lastHeartbeat = millis();
  }

  if (currentMode == "finger_enroll") {
    handleFingerEnroll();
    getDeviceState();
    delay(500);
    return;
  }

  // RFID (Ignoré doucement si débranché)
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.println("RFID UID détecté : " + uid);

    if (WiFi.status() == WL_CONNECTED) {
      if (currentMode == "enroll") {
        handleRFIDEnroll(uid);
      } else {
        handleRFIDAccess(uid);
      }
      getDeviceState();
    }
    delay(500);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  // EMPREINTE (Ignoré proprement si absent)
  if (currentMode == "access" && fingerSensorOK) {
    int fingerId = readFingerprintNonBlocking();
    if (fingerId > 0) {
      if (WiFi.status() == WL_CONNECTED) {
        handleFingerAccess(fingerId);
        getDeviceState();
        delay(500);
      }
    } else if (fingerId == -2) {
      accessDenied();
      while (finger.getImage() != FINGERPRINT_NOFINGER) delay(100);
    }
  }
}
