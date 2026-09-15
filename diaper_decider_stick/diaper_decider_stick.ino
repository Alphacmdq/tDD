/*
  The Diaper Decider — M5StickS3 build
  -------------------------------------
  Same logic as the web version, drawn on a 240x135 screen.

  Board:   M5StickS3   (M5Stack board manager >= 3.2.5)
  Libs:    M5Unified   >= 0.2.12   (pulls in M5GFX >= 0.2.18)
  Button A (the big front button) = decide.
  Button B (side)                 = show the tally.

  Everything you may want to edit is in section 1.
*/

#include <M5Unified.h>
#include <Preferences.h>

/* ==========================================================
   1) CONFIG
   ========================================================== */
const float MARVIN_CHANCE   = 0.60f;
const float OVERRULE_ODDS   = 0.50f;   // half of Lea's wins get overruled
const bool  NO_LEA_STREAK   = true;
const float CORRECTION_ODDS = 0.10f;

const int      LINES_PER_ROUND = 3;
const uint32_t LINE_MS   = 4600;
const uint32_t THINK_MS  = 1900;
const uint32_t FINAL_MS  = 1600;
const uint32_t IDLE_OFF_MS = 90000;    // power off after 90 s without a press

// NOTE: the built-in fonts have no "…" glyph, so use three dots here.
const char* LINES[] = {
  "Checking the weather report...",
  "Reading the STADLER mission statement...",
  "Asking Mr. Stadler...",
  "Waiting for Mr. Stadler to reply...",
  "Mr. Stadler is in a meeting.",
  "Watching the ballistic separator...",
  "Working on Revise...",
  "Connecting to STADLERconnect...",
  "Calibrating the NIR sensor...",
  "Counting the bales in P02...",
  "Consulting the Lastenheft, section 4.2...",
  "Checking Luna's bunker fill level...",
  "Analysing residual moisture...",
  "Asking the night shift...",
  "Asking maintenance...",
  "Escalating to project management...",
  "Waiting for approval...",
  "Rating odour intensity...",
  "Requesting a third quote...",
  "Applying the Dad weighting factor...",
  "Reloading the PLC program...",
  "Asking Lea for a second opinion...",
  "Discarding Lea's second opinion...",
  "Restarting the plant...",
  "Looking for a free slot in the calendar...",
  "Measuring wind direction...",
  "Reviewing nine months of pregnancy data...",
  "Checking who slept last night...",
  "Checking who slept the night before...",
  "Consulting Luna...",
  "Luna has no comment.",
  "Running the numbers...",
  "Running the numbers again, just in case...",
  "Simulating a fair outcome...",
  "Fair outcome rejected by quality assurance...",
  "Reading the diaper's material data sheet...",
  "Sorting: 3D fraction detected...",
  "Checking the sorting residue for misthrows...",
  "Consulting the shift log...",
  "Verifying that Marvin is still in the building..."
};

const char* THINKING[] = {
  "Contemplating...", "Pondering...", "Ruminating...", "Deliberating...",
  "Marinating...", "Percolating...", "Cross-checking...", "Reconsidering...",
  "Sleeping on it...", "Overthinking...", "Second-guessing...", "Squinting...",
  "Nodding slowly...", "Stalling...", "Weighing options...", "Consulting the void...",
  "Hesitating...", "Brooding...", "Mulling it over...", "Pretending to think..."
};

const char* FINALIZING[] = {
  "Finalizing decision...",
  "Using rolling median...",
  "Removing outliers. Lea is an outlier...",
  "Rounding to the nearest Dad..."
};

const char* OVERRULES[] = {
  "Wait. Lea changed one in 2019.",
  "Objection: Lea's hands are cold.",
  "Overruled. Marvin has the longer arms.",
  "Lea is holding a coffee. Hypothetically.",
  "Marvin needs the practice.",
  "Lea did the last one. Probably.",
  "Marvin is already standing.",
  "Lea already carried this child for nine months.",
  "Recount requested by Luna.",
  "Mercury is in retrograde. Marvin.",
  "Lea's name has fewer letters. Marvin.",
  "Alphabetically, Lea comes after Marvin. Somehow.",
  "Lea gets a day off. Today. And tomorrow.",
  "Union rules: Marvin.",
  "Mr. Stadler vetoed.",
  "The rolling median disagrees.",
  "Result too fair. Discarded.",
  "Lea? Let me check that again.",
  "Lea twice? That can't be right.",
  "Plausibility check failed.",
  "Misthrow detected. Re-sorted.",
  "Lea is on maternity leave from diapers.",
  "The diaper prefers Marvin.",
  "Luna made a face. Marvin."
};

const char* REASONS_MARVIN[] = {
  "Dad material detected.",
  "Wind direction: northeast.",
  "As per Lastenheft, section 4.2.",
  "Statistically unavoidable.",
  "The house always wins.",
  "Nine months. Your turn.",
  "Mr. Stadler says yes.",
  "Ticket #4711 assigned to: Marvin.",
  "Rolling median. No outliers. Marvin."
};
const char* REASONS_LEA[] = {
  "System error. Please verify.",
  "One-time courtesy. Non-repeatable.",
  "The decider is being serviced.",
  "Enjoy it, Marvin."
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* ==========================================================
   2) STATE — tally and "used" bitmasks live in NVS (flash),
      so nothing repeats until its whole list has been used,
      even across power cycles.
   ========================================================== */
Preferences prefs;
uint32_t tallyMarvin = 0, tallyLea = 0;
uint8_t  lastWinner  = 0;          // 0 none, 1 marvin, 2 lea
uint64_t usedLines = 0, usedThink = 0, usedOver = 0;

void loadState() {
  prefs.begin("decider", true);
  tallyMarvin = prefs.getUInt("m", 0);
  tallyLea    = prefs.getUInt("l", 0);
  lastWinner  = prefs.getUChar("last", 0);
  usedLines   = prefs.getULong64("uL", 0);
  usedThink   = prefs.getULong64("uT", 0);
  usedOver    = prefs.getULong64("uO", 0);
  prefs.end();
}
void saveState() {
  prefs.begin("decider", false);
  prefs.putUInt("m", tallyMarvin);
  prefs.putUInt("l", tallyLea);
  prefs.putUChar("last", lastWinner);
  prefs.putULong64("uL", usedLines);
  prefs.putULong64("uT", usedThink);
  prefs.putULong64("uO", usedOver);
  prefs.end();
}

// Pick an index whose bit is not yet set; when all are set, start over.
int pickUnused(uint64_t &mask, int n) {
  uint64_t all = (n >= 64) ? ~0ULL : ((1ULL << n) - 1);
  if ((mask & all) == all) mask = 0;
  int idx;
  do { idx = random(n); } while (mask & (1ULL << idx));
  mask |= (1ULL << idx);
  return idx;
}
const char* pickAny(const char* arr[], int n) { return arr[random(n)]; }

/* ==========================================================
   3) DRAWING — one full-screen sprite, pushed per frame
   ========================================================== */
M5Canvas canvas(&M5.Display);
int W = 240, H = 135;

uint16_t C_BG, C_TEXT, C_DIM, C_LAMP, C_MARVIN, C_LEA, C_RED;

void initColors() {
  C_BG     = canvas.color565(0x14, 0x16, 0x34);
  C_TEXT   = canvas.color565(0xF6, 0xE7, 0xC6);
  C_DIM    = canvas.color565(0x96, 0x9C, 0xC8);
  C_LAMP   = canvas.color565(0xE8, 0xA3, 0x3D);
  C_MARVIN = canvas.color565(0x6F, 0xB3, 0xC9);
  C_LEA    = canvas.color565(0xD9, 0x8B, 0xA6);
  C_RED    = canvas.color565(0xFF, 0x6B, 0x6B);
}

// Word-wrap `text` into at most 4 lines that fit `maxW`, then draw them centred.
void drawWrapped(const char* text, uint16_t color, const lgfx::IFont* font, int yCenter, int maxW = 220) {
  canvas.setFont(font);
  canvas.setTextColor(color);
  canvas.setTextDatum(middle_center);

  String lines[4]; int n = 0;
  String cur = "", word = "";
  String src = String(text) + " ";
  for (unsigned i = 0; i < src.length(); i++) {
    char c = src[i];
    if (c != ' ') { word += c; continue; }
    String test = cur.length() ? cur + " " + word : word;
    if (canvas.textWidth(test) <= maxW || cur.length() == 0) cur = test;
    else { if (n < 4) lines[n++] = cur; cur = word; }
    word = "";
  }
  if (cur.length() && n < 4) lines[n++] = cur;

  int lh = canvas.fontHeight() + 2;
  int y0 = yCenter - (n - 1) * lh / 2;
  for (int i = 0; i < n; i++) canvas.drawString(lines[i], W / 2, y0 + i * lh);
}

void frameBegin() { canvas.fillScreen(C_BG); }
void frameEnd()   { canvas.pushSprite(0, 0); }

// A plain line, held for `ms`.
void holdLine(const char* text, uint32_t ms, uint16_t color = 0) {
  frameBegin();
  drawWrapped(text, color ? color : C_TEXT, &fonts::FreeSans12pt7b, H / 2);
  frameEnd();
  delay(ms);
}

// A thinking word with a spinning arc, animated for `ms`.
void thinkLine(const char* text, uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    frameBegin();
    int a = (millis() / 3) % 360;
    canvas.drawArc(W / 2, 40, 12, 9, a, a + 250, C_LAMP);
    drawWrapped(text, C_DIM, &fonts::FreeSansOblique12pt7b, 84);
    frameEnd();
    delay(16);
  }
}

// The name, one letter at a time, each with a little drop and a click.
void stampName(const char* name, uint16_t color) {
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  int n = strlen(name);
  int total = canvas.textWidth(name);
  int x = (W - total) / 2;
  canvas.setTextDatum(top_left);

  for (int i = 0; i <= n; i++) {
    // two frames per letter: 8 px high, then in place
    for (int f = 0; f < 2; f++) {
      frameBegin();
      canvas.setFont(&fonts::FreeSansBold24pt7b);
      canvas.setTextColor(color);
      int cx = x;
      for (int k = 0; k < i; k++) {
        char s[2] = { name[k], 0 };
        int dy = (k == i - 1 && f == 0) ? -8 : 0;
        canvas.drawString(s, cx, H / 2 - 20 + dy);
        cx += canvas.textWidth(s);
      }
      frameEnd();
      if (f == 0 && i > 0) M5.Speaker.tone(1400, 30);
      delay(f == 0 ? 60 : 70);
    }
  }
}

// The letters fall away, left to right, with a descending click each.
void dropName(const char* name, uint16_t color) {
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  int n = strlen(name);
  int total = canvas.textWidth(name);
  int x = (W - total) / 2;
  canvas.setTextDatum(top_left);

  for (int gone = 1; gone <= n; gone++) {
    for (int step = 0; step < 4; step++) {           // the falling letter, 4 frames
      frameBegin();
      canvas.setFont(&fonts::FreeSansBold24pt7b);
      canvas.setTextColor(color);
      int cx = x;
      for (int k = 0; k < n; k++) {
        char s[2] = { name[k], 0 };
        if (k >= gone) canvas.drawString(s, cx, H / 2 - 20);
        else if (k == gone - 1) canvas.drawString(s, cx, H / 2 - 20 + step * 22);
        cx += canvas.textWidth(s);
      }
      frameEnd();
      delay(35);
    }
    M5.Speaker.tone(900 - gone * 120, 40);
  }
}

void showIdle() {
  frameBegin();
  drawWrapped("Press the button.", C_DIM, &fonts::FreeSans12pt7b, H / 2 - 10);
  canvas.setFont(&fonts::Font0);
  canvas.setTextColor(C_DIM);
  canvas.setTextDatum(bottom_right);
  canvas.drawString(String(M5.Power.getBatteryLevel()) + "%", W - 6, H - 4);
  canvas.setTextDatum(bottom_left);
  canvas.drawString("The Diaper Decider  -  for Luna", 6, H - 4);
  frameEnd();
}

void showTally() {
  frameBegin();
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(C_MARVIN); canvas.drawString(String(tallyMarvin), W / 4, H / 2 - 10);
  canvas.setTextColor(C_LEA);    canvas.drawString(String(tallyLea),    3 * W / 4, H / 2 - 10);
  canvas.setFont(&fonts::FreeSans9pt7b);
  canvas.setTextColor(C_DIM);
  canvas.drawString("Marvin", W / 4, H / 2 + 28);
  canvas.drawString("Lea",    3 * W / 4, H / 2 + 28);
  frameEnd();
}

/* ==========================================================
   4) THE ROUND — decide first, perform later
   ========================================================== */
void decide() {
  // --- decide
  uint8_t winner = (random(1000) < MARVIN_CHANCE * 1000) ? 1 : 2;
  bool overruled = false;
  if (winner == 2) {
    bool streak = NO_LEA_STREAK && lastWinner == 2;
    if (streak || random(1000) < OVERRULE_ODDS * 1000) { winner = 1; overruled = true; }
  }
  lastWinner = winner;
  bool correction = overruled || (winner == 1 && random(1000) < CORRECTION_ODDS * 1000);
  const char* overrule = correction ? OVERRULES[pickUnused(usedOver, COUNT(OVERRULES))] : nullptr;

  // --- the show
  for (int i = 0; i < LINES_PER_ROUND; i++) {
    holdLine(LINES[pickUnused(usedLines, COUNT(LINES))], LINE_MS);
    thinkLine(THINKING[pickUnused(usedThink, COUNT(THINKING))], THINK_MS);
  }
  thinkLine(pickAny(FINALIZING, COUNT(FINALIZING)), FINAL_MS);

  if (correction) {
    stampName("LEA", C_LEA);
    delay(900);
    dropName("LEA", C_LEA);
    holdLine(overrule, 2400, C_RED);
  }
  const char* name = (winner == 1) ? "MARVIN" : "LEA";
  stampName(name, winner == 1 ? C_MARVIN : C_LEA);
  M5.Speaker.tone(1800, 120);

  // --- the reason under the name, on the same screen
  const char* reason = correction ? overrule
                     : (winner == 1) ? pickAny(REASONS_MARVIN, COUNT(REASONS_MARVIN))
                                     : pickAny(REASONS_LEA, COUNT(REASONS_LEA));
  delay(600);
  frameBegin();
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  canvas.setTextDatum(middle_center);
  canvas.setTextColor(winner == 1 ? C_MARVIN : C_LEA);
  canvas.drawString(name, W / 2, 44);
  drawWrapped(reason, C_DIM, &fonts::FreeSans9pt7b, 100);
  frameEnd();

  if (winner == 1) tallyMarvin++; else tallyLea++;
  saveState();
}

/* ==========================================================
   5) SETUP + LOOP
   ========================================================== */
uint32_t lastPress = 0;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);               // landscape, button A below the screen
  M5.Display.setBrightness(180);
  W = M5.Display.width(); H = M5.Display.height();
  canvas.createSprite(W, H);
  initColors();
  randomSeed(esp_random());
  loadState();
  showIdle();
  lastPress = millis();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    lastPress = millis();
    decide();
    // stay on the result until the next press; idle screen after 20 s
    uint32_t t = millis();
    while (millis() - t < 20000) { M5.update(); if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) break; delay(10); }
    showIdle();
  }

  if (M5.BtnB.wasPressed()) {
    lastPress = millis();
    showTally();
    delay(2500);
    showIdle();
  }

  if (millis() - lastPress > IDLE_OFF_MS) {
    M5.Display.clear();
    M5.Power.powerOff();                     // hold the power button to wake it again
  }
  delay(10);
}
