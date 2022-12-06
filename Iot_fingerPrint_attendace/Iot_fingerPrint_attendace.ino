
#include <ArduinoJson.h>
#include <Arduino.h>

#include <Wire.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>

#include <Adafruit_Fingerprint.h>

WiFiMulti WiFiMulti;
WebSocketsClient WebSocket;

#define USE_SERIAL Serial
#define mySerial Serial2

uint8_t id;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t* scr = (const uint8_t*) mem;
  USE_SERIAL.printf("\n[HEXDUMP] Address:0x%08X len: 0x%X (%d)", (ptrdiff_t)scr, len, len);
  for (uint32_t i = 0; i < len; i++) {
    if (i % cols == 0) {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)scr, i);
    }
    USE_SERIAL.printf("%02X", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void dataToHandle(uint8_t * text) {
  String dataRecive = (char *)text;

  Serial.println(dataRecive);
  DynamicJsonDocument doc(1024);
  deserializeJson(doc , dataRecive);

  if (doc["command"] == "enroll") {
    Serial.pritln("enrolling");
    id = doc["enid"];
    getFingerprintEnroll();

  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void fingerDetected(int userId) {

  String s;
  DynamicJsonDocument doc(1024);

  doc["userId"] = userId;
  serializeJson(doc, s);

  webSocket.sendTXT(s);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      USE_SERIAL.printf("[WSc] connected to Url:%s\n", payload);

      //send msg to server when connected
      webSocket.sendTXT("connected");
      break;
    ////////////////////////////////functions here ///////////////////////////////////////////////
    case WStype_TEXT:
      dataToHandle(payload);
      break;

    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);

      break;

    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setup() {

  USE_SERIAL.begin(115200);

  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP]BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);

  }
  WiFiMulti.addAP("Dialog 4G 275", "Ïshan1997");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(10);
  }

  //server address ,port and URL
  webSocket.begin("192.168.43.127", 8080, "/");

  //event handler
  webSocket.onEvent(webSocketEvent);

  webSocket.setReconnectInterval(5000);
  ///////////////////////////////////////////////////////////////////////////////////////////////////
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}

void loop(){

  webSocket.loop();

  getFingerprintID();
  delay(100);
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
