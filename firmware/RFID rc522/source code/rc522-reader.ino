/**
 * RFID RC522 Reader — XOR722 X3
 * Tampil di layar TFT ST7789 + Serial Monitor
 *
 * Wiring RC522:
 *   SDA → GPIO 14 | SCK → GPIO 18 | MOSI → GPIO 17
 *   MISO → GPIO 8 | RST → GPIO 6 | 3.3V | GND
 *
 * Library: MFRC522 by GithubCommunity, TFT_eSPI by Bodmer
 * Board  : ESP32S3 Dev Module | USB CDC On Boot: Enabled
 */

#include <SPI.h>
#include <MFRC522.h>
#include <TFT_eSPI.h>

#define RC522_CS   14
#define RC522_RST  6

MFRC522 rfid(RC522_CS, RC522_RST);
MFRC522::MIFARE_Key keyA;
TFT_eSPI tft = TFT_eSPI();

// Hacker cyber green theme
#define C_BG        0x0000
#define C_PANEL     0x0204
#define C_HEADER    0x07E0
#define C_LABEL     0x5FE0
#define C_VALUE     0xAFE5
#define C_OK        0x07E0
#define C_WAIT      0x7BEF
#define C_LINE      0x03E0
#define C_DARKGREEN 0x0180

#define Y_TITLE   8
#define Y_DIVIDER 32
#define Y_TIPE    45
#define Y_UID     78
#define Y_ATQA    111
#define Y_SAK     140

String lastUID = "";

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  drawBase();
  showWaiting();

  SPI.begin(18, 8, 17, RC522_CS);
  delay(100);

  rfid.PCD_Init();
  delay(100);

  rfid.PCD_WriteRegister(MFRC522::TModeReg,      0x80);
  rfid.PCD_WriteRegister(MFRC522::TPrescalerReg, 0xA9);
  rfid.PCD_WriteRegister(MFRC522::TReloadRegH,   0x03);
  rfid.PCD_WriteRegister(MFRC522::TReloadRegL,   0xE8);
  rfid.PCD_WriteRegister(MFRC522::TxASKReg,      0x40);
  rfid.PCD_WriteRegister(MFRC522::ModeReg,       0x3D);

  rfid.PCD_AntennaOn();
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);

  for (byte i = 0; i < 6; i++) keyA.keyByte[i] = 0xFF;

  byte ver = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("RC522 Version: 0x%02X %s\n", ver,
    (ver == 0x00 || ver == 0xFF) ? "[ERROR]" : "[OK]");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String tipe = rfid.PICC_GetTypeName(rfid.PICC_GetType(rfid.uid.sak));

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) uid += " ";
  }
  uid.toUpperCase();

  // Kalau kartunya sama, jangan refresh layar
  if (uid == lastUID) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(300);
    return;
  }

  lastUID = uid;

  String atqa = (rfid.uid.size == 4) ? "00 04" :
                (rfid.uid.size == 7) ? "00 44" : "00 84";

  String sak = "0x";
  if (rfid.uid.sak < 0x10) sak += "0";
  sak += String(rfid.uid.sak, HEX);
  sak.toUpperCase();

  Serial.println("\n════════════ KARTU BARU TERDETEKSI ════════════");
  Serial.println("TIPE : " + tipe);
  Serial.println("UID  : " + uid);
  Serial.println("ATQA : " + atqa);
  Serial.println("SAK  : " + sak);

  showScanning();
  readBlocks();
  showCard(tipe, uid, atqa, sak);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(500);
}

void drawBase() {
  tft.fillScreen(C_BG);

  tft.fillRect(0, 0, 320, 32, C_PANEL);
  tft.drawRect(0, 0, 320, 170, C_DARKGREEN);
  tft.drawRect(3, 3, 314, 164, C_DARKGREEN);

  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(2);
  tft.setTextColor(C_HEADER, C_PANEL);
  tft.drawString("CYBER NFC SCANNER", 160, Y_TITLE);

  tft.drawFastHLine(0, Y_DIVIDER, 320, C_LINE);

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(C_LABEL, C_BG);

  tft.drawString("[ CARD TYPE ]", 10, Y_TIPE);
  tft.drawString("[ UID HASH  ]", 10, Y_UID);
  tft.drawString("[ ATQA      ]", 10, Y_ATQA);
  tft.drawString("[ SAK       ]", 10, Y_SAK);
}

void showWaiting() {
  clearValues();

  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(C_WAIT, C_BG);
  tft.drawString("SYSTEM READY - TAP NFC CARD", 160, 90);

  tft.setTextColor(C_DARKGREEN, C_BG);
  tft.drawString("waiting for target...", 160, 110);
}

void showScanning() {
  tft.fillRect(210, 36, 100, 16, C_BG);
  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(C_OK, C_BG);
  tft.drawString("SCANNING...", 310, 38);
}

void clearValues() {
  tft.fillRect(0, 34, 320, 134, C_BG);
}

void showCard(String tipe, String uid, String atqa, String sak) {
  clearValues();

  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(C_LABEL, C_BG);

  tft.drawString("[ CARD TYPE ]", 10, Y_TIPE);
  tft.drawString("[ UID HASH  ]", 10, Y_UID);
  tft.drawString("[ ATQA      ]", 10, Y_ATQA);
  tft.drawString("[ SAK       ]", 10, Y_SAK);

  tft.setTextColor(C_VALUE, C_BG);

  tft.setTextSize(1);
  tft.drawString(tipe, 130, Y_TIPE);

  tft.drawString(uid, 130, Y_UID);
  tft.drawString(atqa, 130, Y_ATQA);
  tft.drawString(sak, 130, Y_SAK);

  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(C_OK, C_BG);
  tft.drawString("ACCESS GRANTED", 310, 38);

  tft.drawFastHLine(10, 62, 300, C_DARKGREEN);
  tft.drawFastHLine(10, 95, 300, C_DARKGREEN);
  tft.drawFastHLine(10, 128, 300, C_DARKGREEN);
}

void readBlocks() {
  MFRC522::PICC_Type t = rfid.PICC_GetType(rfid.uid.sak);

  if (t != MFRC522::PICC_TYPE_MIFARE_1K &&
      t != MFRC522::PICC_TYPE_MIFARE_4K &&
      t != MFRC522::PICC_TYPE_MIFARE_MINI) return;

  byte totalSectors = (t == MFRC522::PICC_TYPE_MIFARE_4K) ? 40 :
                      (t == MFRC522::PICC_TYPE_MIFARE_MINI) ? 5 : 16;

  Serial.println("\n──── DATA BLOK ────");

  for (byte s = 0; s < totalSectors; s++) {
    byte blokAwal = s * 4;
    byte jumlahBlok = 4;

    if (s >= 32) {
      blokAwal = 128 + (s - 32) * 16;
      jumlahBlok = 16;
    }

    rfid.PCD_StopCrypto1();
    delay(10);

    if (rfid.PCD_Authenticate(
          MFRC522::PICC_CMD_MF_AUTH_KEY_A,
          blokAwal + jumlahBlok - 1,
          &keyA,
          &rfid.uid
        ) != MFRC522::STATUS_OK) {
      Serial.printf("[ S%02d ] AUTH GAGAL\n", s);
      continue;
    }

    Serial.printf("[ S%02d ] AUTH OK\n", s);

    for (byte b = 0; b < jumlahBlok; b++) {
      byte blok = blokAwal + b;
      byte buf[18];
      byte bSize = sizeof(buf);

      if (rfid.MIFARE_Read(blok, buf, &bSize) != MFRC522::STATUS_OK) {
        Serial.printf("  Blok %3d : READ GAGAL\n", blok);
        continue;
      }

      Serial.printf("  Blok %3d : ", blok);

      for (byte i = 0; i < 16; i++) {
        Serial.printf("%02X ", buf[i]);
      }

      Serial.print("| ");

      for (byte i = 0; i < 16; i++) {
        Serial.print((buf[i] >= 32 && buf[i] < 127) ? (char)buf[i] : '.');
      }

      if (blok == 0) Serial.print(" <- Manufacturer");
      else if (b == jumlahBlok - 1) Serial.print(" <- Sector Trailer");

      Serial.println();
    }
  }

  Serial.println("══════════════════════════════════════════");
}
