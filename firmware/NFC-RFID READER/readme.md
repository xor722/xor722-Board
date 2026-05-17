
# 📟 NFC / RFID Reader — XOR722 X3 Board

Program pembaca kartu NFC/RFID menggunakan board **XOR722 X3** + modul **PN532** via I2C. Hasil pembacaan ditampilkan langsung di layar TFT bawaan board.

---

## 🛒 Perangkat yang Dibutuhkan

- XOR722 X3 Board
- Modul PN532 (pastikan mode **I2C** aktif)
- Kartu RFID / tag NFC
- Kabel jumper & kabel USB

---

## 🔌 Koneksi PN532 ke Board

| Pin PN532 | Pin XOR722 X3 |
|-----------|---------------|
| SDA       | GPIO 47       |
| SCL       | GPIO 48       |
| VCC       | 3.3V          |
| GND       | GND           |

> ⚠️ Set jumper/switch PN532 ke mode **I2C** sebelum digunakan.

---

## 📦 Library yang Diperlukan

Install via Arduino IDE Library Manager:

- `TFT_eSPI` — by Bodmer
- `PN532` — by Elechouse
- `NDEF` — (NfcAdapter)

---

## 🚀 Upload Program (Arduino IDE)

1. Board: `ESP32S3 Dev Module`
2. USB Mode: `Hardware CDC and JTAG`
3. USB CDC On Boot: `Enabled`
4. Flash Size: `16MB` / PSRAM: `OPI PSRAM`
5. Klik **Upload**

---

## 🖐️ Cara Pakai

1. Upload program → layar tampil tulisan **"Tempelkan kartu RFID..."**
2. Tempelkan kartu ke modul PN532
3. Layar menampilkan **CARD TYPE** dan **UID** kartu
4. Buka Serial Monitor (115200 baud) untuk melihat log

---
<img width="845" height="1078" alt="circuit_image" src="https://github.com/user-attachments/assets/a9907756-23be-41bc-8cc4-d851f6debd39" />
