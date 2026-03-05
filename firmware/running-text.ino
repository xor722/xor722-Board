#include <TFT_eSPI.h>
#include <Preferences.h>

// ===================== PIN DEFINITIONS =====================
#define BTN_OK    0
#define BTN_UP    7
#define BTN_DOWN  15
#define BTN_RIGHT 38
#define BTN_LEFT  39

#define TFT_MOSI 9
#define TFT_SCLK 11
#define TFT_CS   41
#define TFT_DC   16
#define TFT_RST  40
#define TFT_BL   21

// ===================== DISPLAY SETUP =====================
TFT_eSPI tft = TFT_eSPI();
Preferences preferences;

// ===================== COLORS =====================
#define COLOR_BG        TFT_BLACK
#define COLOR_MENU_SEL  0x07FF
#define COLOR_MENU_NORM TFT_WHITE
#define COLOR_BORDER    0x4208
#define COLOR_KEY_BG    0x2104
#define COLOR_KEY_SEL   0x07FF
#define COLOR_KEY_TEXT  TFT_WHITE
#define COLOR_PREVIEW   TFT_YELLOW

// ===================== SCREEN DIMENSIONS =====================
#define SCR_W 320
#define SCR_H 170

// ===================== STATE MACHINE =====================
enum AppState {
  STATE_MENU,
  STATE_PLAY,
  STATE_WRITE,
  STATE_SETTINGS
};

AppState currentState = STATE_MENU;

// ===================== MENU =====================
int menuIndex = 0;
const char* menuItems[] = {"1. Play", "2. Write", "3. Settings"};
const int MENU_COUNT = 3;

// ===================== RUNNING TEXT =====================
String runningText = "";
uint16_t textColor = TFT_GREEN;
int runX = SCR_W;
unsigned long lastRunUpdate = 0;
int runSpeed = 3;

unsigned long rightHoldStart = 0;
bool rightHolding = false;
bool backToMenuTriggered = false;

// ===================== KEYBOARD =====================
String typedText = "";
int kbRow = 0;
int kbCol = 0;
bool capsLock = false;
bool shiftLayer = false;

const char* kb_normal[4][13] = {
  {"1","2","3","4","5","6","7","8","9","0","-","=",""},
  {"q","w","e","r","t","y","u","i","o","p","[","]",""},
  {"a","s","d","f","g","h","j","k","l",";","'","",""},
  {"z","x","c","v","b","n","m",",",".","/"," ","",""}
};

const char* kb_shift[4][13] = {
  {"!","@","#","$","%","^","&","*","(",")","-","=",""},
  {"Q","W","E","R","T","Y","U","I","O","P","[","]",""},
  {"A","S","D","F","G","H","J","K","L",";","'","",""},
  {"Z","X","C","V","B","N","M",",",".","/"," ","",""}
};

const char* kb_symbol[4][13] = {
  {"!","@","#","$","%","^","&","*","(",")","_","+",""},
  {"`","~","\\","|","{","}","<",">","?","/","\"","",""},
  {"","","","","","","","","","","","",""},
  {"","","","","","","","","","","","",""}
};

const char* kb_ctrl[] = {"CAP","SYM","SPACE","BKSP","ENTER"};
const int KB_CTRL_COUNT = 5;
const int kb_cols[5] = {11, 10, 11, 10, 5};

// ===================== SETTINGS =====================
struct ColorEntry {
  const char* name;
  uint16_t color;
};

const ColorEntry colorTable[] = {
  {"Biru",    TFT_RED},
  {"Hijau",    TFT_GREEN},
  {"Merah",     TFT_BLUE},
  {"Aqua",   TFT_YELLOW},
  {"Kuning",     TFT_CYAN},
  {"Magenta",  TFT_MAGENTA},
  {"Putih",    TFT_WHITE},
  {"Biru2",   0xFD20},
  {"Pink",     0xF81F},
  {"Ungu",     0x780F},
  {"Hijau2",     0x07E0},
  {"Kuning2",     0x07FF},
};
const int COLOR_COUNT = 12;
int colorSelRow = 0;
int colorSelCol = 0;
const int COLOR_COLS = 4;
const int COLOR_ROWS = 3;

// ===================== POPUP =====================
bool showPopup = false;
unsigned long popupTime = 0;
String popupMsg = "";

// ===================== BUTTON DEBOUNCE =====================
struct Button {
  int pin;
  bool lastState;
  bool currentState;
  unsigned long lastDebounce;
  bool pressed;
  bool released;
  bool held;
  unsigned long heldStart;
};

Button buttons[5];

void initButtons() {
  int pins[] = {BTN_OK, BTN_UP, BTN_DOWN, BTN_RIGHT, BTN_LEFT};
  for (int i = 0; i < 5; i++) {
    buttons[i].pin = pins[i];
    buttons[i].lastState = HIGH;
    buttons[i].currentState = HIGH;
    buttons[i].lastDebounce = 0;
    buttons[i].pressed = false;
    buttons[i].released = false;
    buttons[i].held = false;
    buttons[i].heldStart = 0;
    pinMode(pins[i], INPUT_PULLUP);
  }
}

void updateButtons() {
  for (int i = 0; i < 5; i++) {
    bool reading = digitalRead(buttons[i].pin);
    buttons[i].pressed = false;
    buttons[i].released = false;

    if (reading != buttons[i].lastState) {
      buttons[i].lastDebounce = millis();
    }

    if ((millis() - buttons[i].lastDebounce) > 50) {
      if (reading != buttons[i].currentState) {
        buttons[i].currentState = reading;
        if (buttons[i].currentState == LOW) {
          buttons[i].pressed = true;
          buttons[i].heldStart = millis();
          buttons[i].held = false;
        } else {
          buttons[i].released = true;
          buttons[i].held = false;
        }
      }
    }
    buttons[i].lastState = reading;
  }
}

bool btnPressed(int idx)  { return buttons[idx].pressed; }
bool btnReleased(int idx) { return buttons[idx].released; }
bool btnHeld(int idx, unsigned long ms) {
  if (buttons[idx].currentState == LOW) {
    return (millis() - buttons[idx].heldStart) >= ms;
  }
  return false;
}

#define IDX_OK    0
#define IDX_UP    1
#define IDX_DOWN  2
#define IDX_RIGHT 3
#define IDX_LEFT  4

// ===================== FORWARD DECLARATIONS =====================
void drawMenu();
void drawRunningText();
void drawKeyboard();
void drawSettings();
void drawPopup(String msg);
void handleMenu();
void handlePlay();
void handleWrite();
void handleSettings();
void startPlay();
void startWrite();
void startSettings();
void startMenu();
char getKeyChar(int row, int col);
void drawKeyboardKey(int row, int col, bool selected);
void drawAllKeys();

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  initButtons();

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);

  preferences.begin("runtext", false);
  runningText = preferences.getString("text", "");
  textColor   = (uint16_t)preferences.getUInt("color", TFT_GREEN);
  preferences.end();

  startMenu();
}

// ===================== LOOP =====================
void loop() {
  updateButtons();

  switch (currentState) {
    case STATE_MENU:     handleMenu();     break;
    case STATE_PLAY:     handlePlay();     break;
    case STATE_WRITE:    handleWrite();    break;
    case STATE_SETTINGS: handleSettings(); break;
  }

  if (showPopup && (millis() - popupTime > 1500)) {
    showPopup = false;
    startMenu();
  }
}

// ============================================================
//                        MENU
// ============================================================
void startMenu() {
  currentState = STATE_MENU;
  menuIndex = 0;
  tft.fillScreen(COLOR_BG);
  drawMenu();
}

void drawMenu() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(90, 10);
  tft.print("RUNNING TEXT");
  tft.drawLine(0, 32, SCR_W, 32, TFT_CYAN);

  for (int i = 0; i < MENU_COUNT; i++) {
    int y = 50 + i * 35;
    if (i == menuIndex) {
      tft.fillRoundRect(60, y - 5, 200, 28, 5, COLOR_MENU_SEL);
      tft.setTextColor(TFT_BLACK, COLOR_MENU_SEL);
    } else {
      tft.fillRoundRect(60, y - 5, 200, 28, 5, COLOR_BORDER);
      tft.setTextColor(COLOR_MENU_NORM, COLOR_BORDER);
    }
    tft.setTextSize(2);
    int tx = 60 + (200 - strlen(menuItems[i]) * 12) / 2;
    tft.setCursor(tx, y);
    tft.print(menuItems[i]);
  }
}

void handleMenu() {
  bool changed = false;

  if (btnPressed(IDX_UP)) {
    menuIndex = (menuIndex - 1 + MENU_COUNT) % MENU_COUNT;
    changed = true;
  }
  if (btnPressed(IDX_DOWN)) {
    menuIndex = (menuIndex + 1) % MENU_COUNT;
    changed = true;
  }
  if (changed) drawMenu();

  if (btnPressed(IDX_OK)) {
    switch (menuIndex) {
      case 0:
        if (runningText.length() == 0) startWrite();
        else startPlay();
        break;
      case 1: startWrite();    break;
      case 2: startSettings(); break;
    }
  }
}

// ============================================================
//  PLAY (RUNNING TEXT)
//  - Teks diperbesar ~60% dari size3 → size5 (size3=24px, size5=40px)
//  - Posisi vertikal tengah layar
//  - Border digambar SEKALI saat startPlay(), tidak diulang di loop
//  - Area teks di-clear hanya bagian dalam border agar tidak double
// ============================================================

// Area teks: antara border atas dan info bawah
// Border: 2px, info hint: 14px dari bawah
#define PLAY_BORDER_T  2           // top border Y
#define PLAY_TEXT_Y    2           // mulai clear dari sini
#define PLAY_HINT_H    16          // tinggi area hint di bawah
#define PLAY_TEXT_H    (SCR_H - PLAY_HINT_H - 4)  // tinggi area teks bersih
// Untuk setTextSize(5): tinggi karakter = 5*8 = 40px
#define PLAY_FONT_SIZE 5
#define PLAY_CHAR_H    40          // tinggi karakter size5
#define PLAY_CHAR_W    30          // lebar karakter size5 (approx)

void drawPlayScreenStatic() {
  // Gambar elemen statis sekali saja (border + hint)
  tft.fillScreen(COLOR_BG);
  tft.drawRect(0, 0, SCR_W, SCR_H, TFT_CYAN);
  // Garis pemisah hint
  tft.drawLine(1, SCR_H - PLAY_HINT_H - 1, SCR_W - 2, SCR_H - PLAY_HINT_H - 1, 0x2104);
  tft.setTextColor(TFT_DARKGREY, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(4, SCR_H - PLAY_HINT_H + 2);
  tft.print("Tahan RIGHT 2 detik untuk kembali");
}

void drawRunningText() {
  // Area bersih: dalam border, di atas garis hint
  // x: 1..SCR_W-2, y: 1..SCR_H-PLAY_HINT_H-2
  int clearY = 1;
  int clearH  = SCR_H - PLAY_HINT_H - 2;

  // Clear area teks saja (tanpa border & hint)
  tft.fillRect(1, clearY, SCR_W - 2, clearH, COLOR_BG);

  // Hitung posisi Y tengah dalam area bersih
  int textY = clearY + (clearH - PLAY_CHAR_H) / 2;

  tft.setTextWrap(false);

  tft.setTextColor(textColor, COLOR_BG);
  tft.setTextSize(PLAY_FONT_SIZE);
  tft.setCursor(runX, textY);
  tft.print(runningText);
}



void startPlay() {
  currentState = STATE_PLAY;
  runX = SCR_W;          // Mulai dari luar kanan layar
  lastRunUpdate = 0;     // Reset timer agar langsung update
  backToMenuTriggered = false;
  rightHolding = false;
  drawPlayScreenStatic();
}

void handlePlay() {
  if (millis() - lastRunUpdate > runSpeed) {
    lastRunUpdate = millis();
    
    // Hitung lebar teks secara lebih akurat
    int textWidth = runningText.length() * PLAY_CHAR_W;
    
    runX -= 2;
    
    // Reset ketika teks sudah sepenuhnya keluar dari sisi kiri
    // runX + textWidth < 0 berarti seluruh teks sudah lewat layar kiri
    if (runX + textWidth < 0) {
      runX = SCR_W;    // Mulai lagi dari luar kanan
    }
    
    drawRunningText();
  }

  // Deteksi tahan RIGHT 2 detik
  if (buttons[IDX_RIGHT].currentState == LOW) {
    if (!rightHolding) {
      rightHolding = true;
      rightHoldStart = millis();
    }
    if ((millis() - rightHoldStart) >= 2000 && !backToMenuTriggered) {
      backToMenuTriggered = true;
      startMenu();
      return;
    }
  } else {
    rightHolding = false;
  }
}

// ============================================================
//  KEYBOARD / WRITE
//  Layout didesain ulang agar memenuhi layar 320x170:
//  - Preview: y 0..32 (tinggi 33px)
//  - Keyboard: y 33..170, memenuhi lebar 320px dengan margin minimal
// ============================================================

// Preview area
#define PREVIEW_H   33

// Keyboard layout
// Lebar total keyboard = SCR_W - 2*margin
// Gunakan margin 2px kiri-kanan
#define KB_MARGIN_X  2
#define KB_MARGIN_Y  36   // Y mulai keyboard (sedikit di bawah preview)
#define KB_AVAIL_W   (SCR_W - 2 * KB_MARGIN_X)   // 316px
#define KB_AVAIL_H   (SCR_H - KB_MARGIN_Y - 1)   // sampai bawah layar

// Row 0..3: 4 baris karakter
// Row 4: baris kontrol
// Total 5 baris dengan gap

// Hitung KEY_H agar 5 baris + gap pas di KB_AVAIL_H
// 5 baris + 4 gap = KB_AVAIL_H  → KEY_H = (KB_AVAIL_H - 4*gap) / 5
#define KB_GAP_Y_NEW  2
#define KEY_H_NEW     ((KB_AVAIL_H - 4 * KB_GAP_Y_NEW) / 5)  // ~(133-8)/5 = 25px

// Row 0: 11 keys, Row 1: 10 keys, Row 2: 11 keys, Row 3: 10 keys
// Row 4: 5 keys kontrol (lebar variabel)

// Hitung KEY_W berdasarkan row terbanyak (11 key)
// 11 keys + 10 gap = KB_AVAIL_W → KEY_W = (KB_AVAIL_W - 10*gap) / 11
#define KB_GAP_X_NEW  2
#define KEY_W_NEW     ((KB_AVAIL_W - 10 * KB_GAP_X_NEW) / 11)  // ~(316-20)/11 = 26px

// Lebar total untuk row 11-key dan 10-key
// Row 11-key: total = 11*KEY_W_NEW + 10*gap
// Row 10-key: sama tapi offset agar center
// Offset baris 1,3 (10 key): geser separuh key ke kanan agar center
#define ROW_10_OFFSET  ((KEY_W_NEW + KB_GAP_X_NEW) / 2)

// Baris kontrol: lebar dibagi proporsional
// Total = KB_AVAIL_W, 5 tombol + 4 gap
// CAP:42, SYM:42, SPACE:100, BKSP:64, ENTER:64 → sum=312 + 4*2=320 → pas!
// Sesuaikan agar total = KB_AVAIL_W
int ctrl_w[5];  // dihitung di setup

void computeCtrlWidths() {
  // Proporsi: CAP=13, SYM=13, SPACE=31, BKSP=20, ENTER=20  (total=97 unit)
  // Total piksel = KB_AVAIL_W - 4*KB_GAP_X_NEW
  int totalPx = KB_AVAIL_W - 4 * KB_GAP_X_NEW;
  int units[5] = {13, 13, 31, 20, 20};
  int totalUnits = 97;
  int used = 0;
  for (int i = 0; i < 4; i++) {
    ctrl_w[i] = (units[i] * totalPx) / totalUnits;
    used += ctrl_w[i];
  }
  ctrl_w[4] = totalPx - used; // sisa untuk ENTER
}

int getKeyX_new(int row, int col) {
  if (row == 4) {
    int x = KB_MARGIN_X;
    for (int i = 0; i < col; i++) {
      x += ctrl_w[i] + KB_GAP_X_NEW;
    }
    return x;
  }
  // Row 0,2 = 11 key, mulai dari KB_MARGIN_X
  // Row 1,3 = 10 key, mulai dari KB_MARGIN_X + ROW_10_OFFSET
  int startX = KB_MARGIN_X;
  if (row == 1 || row == 3) startX += ROW_10_OFFSET;
  return startX + col * (KEY_W_NEW + KB_GAP_X_NEW);
}

int getKeyY_new(int row) {
  return KB_MARGIN_Y + row * (KEY_H_NEW + KB_GAP_Y_NEW);
}

void drawPreview() {
  tft.fillRect(0, 0, SCR_W, PREVIEW_H, 0x1082);
  tft.drawRect(0, 0, SCR_W, PREVIEW_H, TFT_CYAN);

  tft.setTextColor(COLOR_PREVIEW, 0x1082);
  tft.setTextSize(1);

  String preview = typedText;
  int maxChars = (SCR_W - 10) / 6;
  if ((int)preview.length() > maxChars) {
    preview = preview.substring(preview.length() - maxChars);
  }
  // Vertikal center di preview
  int ty = (PREVIEW_H - 8) / 2;
  tft.setCursor(5, ty);
  tft.print(preview);

  // Kursor
  int cx = 5 + preview.length() * 6;
  if (cx < SCR_W - 4)
    tft.fillRect(cx, ty, 2, 12, TFT_WHITE);
}

void drawKeyboardKey_new(int row, int col, bool selected) {
  int x = getKeyX_new(row, col);
  int y = getKeyY_new(row);
  int w = (row == 4) ? ctrl_w[col] : KEY_W_NEW;
  int h = KEY_H_NEW;

  uint16_t bg = selected ? COLOR_KEY_SEL : COLOR_KEY_BG;
  uint16_t fg = selected ? TFT_BLACK : COLOR_KEY_TEXT;

  tft.fillRoundRect(x, y, w, h, 3, bg);
  tft.drawRoundRect(x, y, w, h, 3, selected ? TFT_WHITE : 0x4208);
  tft.setTextColor(fg, bg);
  tft.setTextSize(1);

  if (row == 4) {
    const char* label = kb_ctrl[col];
    uint16_t lc = fg;
    if (col == 0 && capsLock)   lc = selected ? TFT_BLACK : TFT_YELLOW;
    if (col == 1 && shiftLayer) lc = selected ? TFT_BLACK : TFT_YELLOW;
    tft.setTextColor(lc, bg);
    int lw = strlen(label) * 6;
    int tx = x + (w - lw) / 2;
    int ty2 = y + (h - 8) / 2;
    tft.setCursor(tx, ty2);
    tft.print(label);
  } else {
    char ch = getKeyChar(row, col);
    if (ch != 0 && ch != ' ') {
      char s[2] = {ch, 0};
      int tx = x + (KEY_W_NEW - 6) / 2;
      int ty2 = y + (KEY_H_NEW - 8) / 2;
      tft.setCursor(tx, ty2);
      tft.print(s);
    }
  }
}

void drawAllKeys() {
  for (int r = 0; r < 4; r++) {
    int cols = kb_cols[r];
    for (int c = 0; c < cols; c++) {
      const char* s = shiftLayer ? kb_symbol[r][c]
                    : (capsLock  ? kb_shift[r][c]
                                 : kb_normal[r][c]);
      if (strlen(s) == 0) continue;
      bool sel = (r == kbRow && c == kbCol);
      drawKeyboardKey_new(r, c, sel);
    }
  }
  for (int c = 0; c < KB_CTRL_COUNT; c++) {
    bool sel = (kbRow == 4 && c == kbCol);
    drawKeyboardKey_new(4, c, sel);
  }
}

void drawKeyboardScreen() {
  tft.fillScreen(COLOR_BG);
  // Garis batas bawah preview / atas keyboard
  tft.drawLine(0, PREVIEW_H, SCR_W, PREVIEW_H, 0x4208);
  drawPreview();
  drawAllKeys();
}

void startWrite() {
  currentState = STATE_WRITE;
  typedText = runningText;
  kbRow = 1;
  kbCol = 0;
  capsLock = false;
  shiftLayer = false;
  computeCtrlWidths();
  tft.fillScreen(COLOR_BG);
  drawKeyboardScreen();
}

char getKeyChar(int row, int col) {
  if (row >= 4) return 0;
  const char* s;
  if (shiftLayer)     s = kb_symbol[row][col];
  else if (capsLock)  s = kb_shift[row][col];
  else                s = kb_normal[row][col];
  if (strlen(s) == 0) return 0;
  if (strcmp(s, " ") == 0) return ' ';
  return s[0];
}

int getValidCols(int row) {
  if (row == 4) return KB_CTRL_COUNT;
  int fixed[] = {11, 10, 11, 10, 5};
  return fixed[row];
}

void handleWrite() {
  bool redrawAll = false;
  bool redrawPreview = false;

  if (btnPressed(IDX_RIGHT)) {
    int oldRow = kbRow, oldCol = kbCol;
    int maxCol = getValidCols(kbRow) - 1;
    if (kbCol < maxCol) kbCol++;
    else { kbCol = 0; kbRow = (kbRow + 1) % 5; }
    if (kbRow < 4) {
      char ch = getKeyChar(kbRow, kbCol);
      if (ch == 0) { kbCol = 0; kbRow = (kbRow + 1) % 5; }
    }
    drawKeyboardKey_new(oldRow, oldCol, false);
    drawKeyboardKey_new(kbRow, kbCol, true);
  }

  if (btnPressed(IDX_LEFT)) {
    int oldRow = kbRow, oldCol = kbCol;
    if (kbCol > 0) kbCol--;
    else { kbRow = (kbRow - 1 + 5) % 5; kbCol = getValidCols(kbRow) - 1; }
    if (kbRow < 4) {
      char ch = getKeyChar(kbRow, kbCol);
      if (ch == 0 && kbCol > 0) kbCol--;
    }
    drawKeyboardKey_new(oldRow, oldCol, false);
    drawKeyboardKey_new(kbRow, kbCol, true);
  }

  if (btnPressed(IDX_UP)) {
    int oldRow = kbRow, oldCol = kbCol;
    kbRow = (kbRow - 1 + 5) % 5;
    int maxCol = getValidCols(kbRow) - 1;
    if (kbCol > maxCol) kbCol = maxCol;
    drawKeyboardKey_new(oldRow, oldCol, false);
    drawKeyboardKey_new(kbRow, kbCol, true);
  }

  if (btnPressed(IDX_DOWN)) {
    int oldRow = kbRow, oldCol = kbCol;
    kbRow = (kbRow + 1) % 5;
    int maxCol = getValidCols(kbRow) - 1;
    if (kbCol > maxCol) kbCol = maxCol;
    drawKeyboardKey_new(oldRow, oldCol, false);
    drawKeyboardKey_new(kbRow, kbCol, true);
  }

  if (btnPressed(IDX_OK)) {
    if (kbRow == 4) {
      switch (kbCol) {
        case 0:
          capsLock = !capsLock;
          shiftLayer = false;
          redrawAll = true;
          break;
        case 1:
          shiftLayer = !shiftLayer;
          capsLock = false;
          redrawAll = true;
          break;
        case 2:
          typedText += ' ';
          redrawPreview = true;
          break;
        case 3:
          if (typedText.length() > 0)
            typedText.remove(typedText.length() - 1);
          redrawPreview = true;
          break;
        case 4:
          if (typedText.length() > 0) {
            preferences.begin("runtext", false);
            preferences.putString("text", typedText);
            preferences.end();
            runningText = typedText;
            startPlay();
            return;
          }
          break;
      }
    } else {
      char ch = getKeyChar(kbRow, kbCol);
      if (ch != 0) {
        typedText += ch;
        redrawPreview = true;
      }
    }
  }

  if (redrawAll) {
    tft.fillRect(0, PREVIEW_H + 1, SCR_W, SCR_H - PREVIEW_H - 1, COLOR_BG);
    drawAllKeys();
    drawPreview();
  } else if (redrawPreview) {
    drawPreview();
  }
}

// ============================================================
//                        SETTINGS
// ============================================================
void startSettings() {
  currentState = STATE_SETTINGS;
  colorSelRow = 0;
  colorSelCol = 0;
  tft.fillScreen(COLOR_BG);
  drawSettings();
}

void drawSettings() {
  tft.fillScreen(COLOR_BG);

  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.setTextSize(2);
  tft.setCursor(70, 5);
  tft.print("Pilih Warna Teks");
  tft.drawLine(0, 24, SCR_W, 24, TFT_CYAN);

  int cellW = 70;
  int cellH = 38;
  int startX = 10;
  int startY = 30;

  for (int r = 0; r < COLOR_ROWS; r++) {
    for (int c = 0; c < COLOR_COLS; c++) {
      int idx = r * COLOR_COLS + c;
      if (idx >= COLOR_COUNT) break;

      int x = startX + c * (cellW + 5);
      int y = startY + r * (cellH + 4);
      bool selected = (r == colorSelRow && c == colorSelCol);

      tft.fillRoundRect(x, y, cellW, cellH, 5,
                        selected ? TFT_WHITE : COLOR_BORDER);
      tft.fillRoundRect(x + 2, y + 2, cellW - 4, 18, 3,
                        colorTable[idx].color);

      tft.fillRect(x + 1, y + 21, cellW - 2, 14,
                   selected ? TFT_WHITE : COLOR_BORDER);
      tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE,
                       selected ? TFT_WHITE : COLOR_BORDER);
      tft.setTextSize(1);
      int tx = x + (cellW - strlen(colorTable[idx].name) * 6) / 2;
      tft.setCursor(tx, y + 24);
      tft.print(colorTable[idx].name);

      if (selected) {
        tft.drawRoundRect(x - 1, y - 1, cellW + 2, cellH + 2, 5, TFT_YELLOW);
        tft.drawRoundRect(x - 2, y - 2, cellW + 4, cellH + 4, 5, TFT_YELLOW);
      }
    }
  }

  tft.setTextColor(TFT_DARKGREY, COLOR_BG);
  tft.setTextSize(1);
  tft.setCursor(5, SCR_H - 12);
  tft.print("OK=Pilih  Warna saat ini: ");
  tft.setTextColor(textColor, COLOR_BG);
  for (int i = 0; i < COLOR_COUNT; i++) {
    if (colorTable[i].color == textColor) {
      tft.print(colorTable[i].name);
      break;
    }
  }
}

void handleSettings() {
  if (showPopup) return;
  bool changed = false;

  if (btnPressed(IDX_RIGHT)) { colorSelCol = (colorSelCol + 1) % COLOR_COLS; changed = true; }
  if (btnPressed(IDX_LEFT))  { colorSelCol = (colorSelCol - 1 + COLOR_COLS) % COLOR_COLS; changed = true; }
  if (btnPressed(IDX_DOWN))  { colorSelRow = (colorSelRow + 1) % COLOR_ROWS; changed = true; }
  if (btnPressed(IDX_UP))    { colorSelRow = (colorSelRow - 1 + COLOR_ROWS) % COLOR_ROWS; changed = true; }

  if (changed) drawSettings();

  if (btnPressed(IDX_OK)) {
    int idx = colorSelRow * COLOR_COLS + colorSelCol;
    if (idx < COLOR_COUNT) {
      textColor = colorTable[idx].color;
      preferences.begin("runtext", false);
      preferences.putUInt("color", (uint32_t)textColor);
      preferences.end();
      showPopup = true;
      popupTime = millis();
      drawPopup("Warna dipilih!");
    }
  }
}

// ============================================================
//                        POPUP
// ============================================================
void drawPopup(String msg) {
  int pw = 180, ph = 60;
  int px = (SCR_W - pw) / 2;
  int py = (SCR_H - ph) / 2;

  tft.fillRoundRect(px + 4, py + 4, pw, ph, 8, 0x2104);
  tft.fillRoundRect(px, py, pw, ph, 8, TFT_WHITE);
  tft.drawRoundRect(px, py, pw, ph, 8, TFT_CYAN);
  tft.drawRoundRect(px + 1, py + 1, pw - 2, ph - 2, 7, TFT_CYAN);

  tft.fillCircle(px + 25, py + ph / 2, 15, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  tft.setTextSize(2);
  tft.setCursor(px + 18, py + ph / 2 - 8);
  tft.print("OK");

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  int tx = px + 45 + (pw - 45 - msg.length() * 12) / 2;
  tft.setCursor(tx, py + ph / 2 - 8);
  tft.print(msg);
}
