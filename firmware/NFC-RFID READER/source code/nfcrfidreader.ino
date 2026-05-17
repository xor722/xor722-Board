#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

// =======================
// PIN PN532 I2C
// =======================
#define SDA_PIN 47
#define SCL_PIN 48

// =======================
// PIN BACKLIGHT TFT
// =======================
#define TFT_BL 21

// =======================
// OBJECT
// =======================
TFT_eSPI tft = TFT_eSPI();

PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

String lastUid = "";
String lastType = "";

// =======================
// TAMPILAN HEADER
// =======================
void drawHeader() {
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, tft.width(), 35, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("RFID READER", tft.width() / 2, 18, 4);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Tempelkan kartu RFID...", 10, 55, 2);
}

// =======================
// TAMPILKAN DATA KARTU
// =======================
void showCardData(String type, String uid) {
  // Bersihkan area data saja, header tidak hilang
  tft.fillRect(0, 45, tft.width(), tft.height() - 45, TFT_BLACK);

  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("CARD TYPE:", 10, 55, 2);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(type, 10, 80, 2);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("UID:", 10, 120, 2);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(uid, 10, 145, 2);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("System initialized");

  // Backlight TFT
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // TFT
  tft.init();
  tft.setRotation(1);
  drawHeader();

  // I2C PN532
  Wire.begin(SDA_PIN, SCL_PIN);

  // NFC
  nfc.begin();

  Serial.println("PN532 NFC Reader ready");
}

void loop() {
  readNFC();
}

void readNFC() {
  if (nfc.tagPresent()) {
    NfcTag tag = nfc.read();

    String uid = tag.getUidString();
    String type = tag.getTagType();

    Serial.println("======================");
    Serial.print("Type: ");
    Serial.println(type);
    Serial.print("UID : ");
    Serial.println(uid);

    // Update layar hanya kalau kartu berbeda
    if (uid != lastUid) {
      lastUid = uid;
      lastType = type;

      showCardData(type, uid);
    }
  }

  delay(300);
}
