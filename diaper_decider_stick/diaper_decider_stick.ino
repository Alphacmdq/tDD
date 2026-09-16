/*
  The Diaper Decider — M5StickS3 build
  -------------------------------------
  Same logic as the web version, drawn on a 240x135 screen.

  Board:   M5StickS3   (M5Stack board manager >= 3.2.5)
  Libs:    M5Unified >= 0.2.12 (pulls in M5GFX), M5PM1, SparkFun BMI270
  Button A (front) = decide.   Button B (side) = tally.
  Shake the device to wake it up.

  Everything you may want to edit is in section 1.
*/

#include <M5Unified.h>
#include <Preferences.h>
#include <Wire.h>
#include <M5PM1.h>
#include "SparkFun_BMI270_Arduino_Library.h"

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
const uint32_t RESULT_MS = 20000;      // verdict stays this long, then idle screen
const uint32_t IDLE_OFF_MS = 90000;    // idle → sleep (shake or power button wakes it)

// Shake sensitivity. Threshold: 1 LSB = 0.48 mg. 0x400 ≈ 0.5 g — a real shake,
// not a walk. Duration: 1 LSB = 20 ms. Lower threshold = more sensitive.
const uint16_t SHAKE_THRESHOLD = 0x400;
const uint16_t SHAKE_DURATION  = 0x0A;

// NOTE: built-in fonts have no "…" glyph, so use three dots.
// Every list below is capped at 128 entries (one bit each in the no-repeat mask).
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
  "Verifying that Marvin is still in the building...",
  "Opening a ticket in Revise...",
  "Closing the ticket. Nobody read it.",
  "Downloading the SCADA logs...",
  "The SCADA logs are unhelpful.",
  "Checking the infeed belt speed...",
  "Checking the baler. The baler is fine.",
  "Asking the IPC. The IPC is thinking.",
  "Pinging the Krsko test centre...",
  "Krsko says: not our problem.",
  "Consulting the P&ID...",
  "Consulting the P&ID upside down...",
  "Measuring the diaper's throughput...",
  "Booking a meeting room to decide...",
  "Meeting room is taken.",
  "Asking HR whether this counts as overtime...",
  "HR says no.",
  "Checking Lea's sick note. There is none.",
  "Warming up the windshifter...",
  "Cross-referencing the diaper with the changelog...",
  "Reading the safety data sheet. Twice.",
  "Estimating time to next diaper...",
  "Estimate: soon.",
  "Consulting the Ballistikseparator manual, page 4...",
  "Page 4 is missing.",
  "Checking public holidays in Slovenia...",
  "Slovenia has a holiday. Diapers don't.",
  "Drying clothes with the mini Balli...",
  "The mini Balli has sorted the socks.",
  "Asking Santa Claus...",
  "Santa says he'll get back to us.",
  "Consulting Sky...",
  "Sky (the dog) has no strong opinion.",
  "Feeding Sky...",
  "Sky has been fed. Twice. Allegedly.",
  "Calculating Bastian's sugar consumption...",
  "Bastian's sugar consumption: unchanged.",
  "Taking vitamin D...",
  "Vitamin D taken. Mood unchanged.",
  "Trying the new Chinese place...",
  "The new Chinese place is closed on Mondays.",
  "Checking if Bielefeld is a real place...",
  "Bielefeld could not be verified.",
  "Checking the Aachen weather...",
  "Aachen weather: still raining.",
  "Checking Deutsche Bahn delays...",
  "Deutsche Bahn is delayed. As a concept.",
  "Watering the office plant...",
  "The office plant has opinions.",
  "Checking the vending machine...",
  "Vending machine: out of Snickers.",
  "Warming up the coffee machine...",
  "Coffee machine says: descale me.",
  "Reading the horoscope for Capricorn...",
  "Horoscope says: Marvin.",
  "Counting the steps to the changing table...",
  "Fourteen steps. Fifteen with a baby.",
  "Translating diaper into German...",
  "Windel. Done.",
  "Looking for the TV remote...",
  "The remote is in the fridge.",
  "Asking the neighbours...",
  "The neighbours are asleep. Lucky them.",
  "Checking if the moon is full...",
  "Luna is. The moon isn't.",
  "Checking public holidays in Austria too...",
  "Renewing the parking permit...",
  "Defrosting the freezer...",
  "Looking up the plural of diaper...",
  "Checking the Bundesliga table...",
  "Bundesliga table: irrelevant. Still checked.",
  "Preheating the oven for no reason...",
  "Checking whether it's Friday...",
  "It is not Friday."
};

const char* THINKING[] = {
  "Contemplating...", "Pondering...", "Ruminating...", "Deliberating...",
  "Marinating...", "Percolating...", "Cross-checking...", "Reconsidering...",
  "Sleeping on it...", "Overthinking...", "Second-guessing...", "Squinting...",
  "Nodding slowly...", "Stalling...", "Weighing options...", "Consulting the void...",
  "Hesitating...", "Brooding...", "Mulling it over...", "Pretending to think...",
  "Frowning...", "Stroking chin...", "Staring into the middle distance...",
  "Buffering...", "Rebooting conscience...", "Consulting gut feeling...",
  "Gut feeling unavailable...", "Reading tea leaves...", "Flipping a mental coin...",
  "Ignoring the coin...", "Composing myself...", "Humming...",
  "Pacing...", "Sighing deeply...", "Recalculating...", "Almost there...",
  "Not quite there...", "Checking notes...", "Losing notes...", "Improvising...",
  "Yawning...", "Blinking...", "Adjusting glasses...", "Clearing throat...",
  "Counting to ten...", "Counting to eleven...", "Shrugging...", "Refilling coffee...",
  "Thinking in German...", "Thinking in Slovenian...", "Thinking in Turkish...",
  "Rethinking...", "Unthinking...", "Loading opinion...", "Opinion loaded.",
  "Waiting for a sign...", "Sign received.", "Squinting harder...",
  "Taking a deep breath...", "Holding that breath...", "Consulting Sky...",
  "Sky says nothing.", "Petting Sky...", "Checking the ceiling...",
  "Scrolling...", "Doomscrolling...", "Regretting the scroll..."
};

const char* FINALIZING[] = {
  "Finalizing decision...",
  "Using rolling median...",
  "Removing outliers. Lea is an outlier...",
  "Rounding to the nearest Dad...",
  "Applying safety factor 1.5...",
  "Signing off the decision...",
  "Stamping the decision...",
  "Laminating the decision...",
  "Sending the decision for approval...",
  "Approval received. From myself.",
  "Converting result to Dad units...",
  "Double-checking the obvious..."
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
  "Luna made a face. Marvin.",
  "Lea's shift ended nine months ago.",
  "Marvin asked for more responsibility. Granted.",
  "Sensor drift. Corrected towards Marvin.",
  "Lea is busy being right about something.",
  "The decider was shaken. Marvin.",
  "Lea won. Marvin gets the trophy.",
  "Decision reviewed by the night shift: Marvin.",
  "Statistically, Marvin was due."
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
  "Rolling median. No outliers. Marvin.",
  "3D fraction. Rolls downhill. Like responsibility.",
  "Luna was consulted. Luna agrees.",
  "The baler is fine. Marvin is up.",
  "Approved by the Verfahrenstechnik.",
  "Marvin has the longer arms.",
  "Wipes are on the left. Go.",
  "The PLC agrees.",
  "This was decided before you pressed the button.",
  "Lea already said no. Politely.",
  "Sorted. Bunker: Marvin.",
  "Throughput requires it."
};
const char* REASONS_LEA[] = {
  "System error. Please verify.",
  "One-time courtesy. Non-repeatable.",
  "The decider is being serviced.",
  "Enjoy it, Marvin.",
  "Lea. Do not get used to it."
};

#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* ==========================================================
   2) STATE — tally and "used" bitsets live in NVS (flash),
      so nothing repeats until its whole list has been used.
      A bitset is 16 bytes = 128 entries per list.
   ========================================================== */
Preferences prefs;
uint32_t tallyMarvin = 0, tallyLea = 0;
uint8_t  lastWinner  = 0;          // 0 none, 1 marvin, 2 lea

const int MASK_BYTES = 16;
uint8_t usedLines[MASK_BYTES] = {0}, usedThink[MASK_BYTES] = {0}, usedFinal[MASK_BYTES] = {0},
        usedOver[MASK_BYTES]  = {0}, usedReason[MASK_BYTES] = {0};

void loadState() {
  prefs.begin("decider", true);
  tallyMarvin = prefs.getUInt("m", 0);
  tallyLea    = prefs.getUInt("l", 0);
  lastWinner  = prefs.getUChar("last", 0);
  prefs.getBytes("bL", usedLines,  MASK_BYTES);
  prefs.getBytes("bT", usedThink,  MASK_BYTES);
  prefs.getBytes("bF", usedFinal,  MASK_BYTES);
  prefs.getBytes("bO", usedOver,   MASK_BYTES);
  prefs.getBytes("bR", usedReason, MASK_BYTES);
  prefs.end();
}
void saveState() {
  prefs.begin("decider", false);
  prefs.putUInt("m", tallyMarvin);
  prefs.putUInt("l", tallyLea);
  prefs.putUChar("last", lastWinner);
  prefs.putBytes("bL", usedLines,  MASK_BYTES);
  prefs.putBytes("bT", usedThink,  MASK_BYTES);
  prefs.putBytes("bF", usedFinal,  MASK_BYTES);
  prefs.putBytes("bO", usedOver,   MASK_BYTES);
  prefs.putBytes("bR", usedReason, MASK_BYTES);
  prefs.end();
}

inline bool bitGet(const uint8_t* m, int i) { return m[i >> 3] & (1 << (i & 7)); }
inline void bitSet(uint8_t* m, int i)       { m[i >> 3] |= (1 << (i & 7)); }

// Pick an index not yet used; when every index is used, start the cycle over.
int pickUnused(uint8_t* mask, int n) {
  if (n > MASK_BYTES * 8) n = MASK_BYTES * 8;
  int free = 0;
  for (int i = 0; i < n; i++) if (!bitGet(mask, i)) free++;
  if (free == 0) { memset(mask, 0, MASK_BYTES); free = n; }
  int k = random(free);                    // the k-th unused entry
  for (int i = 0; i < n; i++) {
    if (bitGet(mask, i)) continue;
    if (k-- == 0) { bitSet(mask, i); return i; }
  }
  return 0;                                // unreachable
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

void holdLine(const char* text, uint32_t ms, uint16_t color = 0) {
  frameBegin();
  drawWrapped(text, color ? color : C_TEXT, &fonts::FreeSans12pt7b, H / 2);
  frameEnd();
  delay(ms);
}

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

void stampName(const char* name, uint16_t color) {
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  int n = strlen(name);
  int total = canvas.textWidth(name);
  int x = (W - total) / 2;
  canvas.setTextDatum(top_left);

  for (int i = 0; i <= n; i++) {
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

void dropName(const char* name, uint16_t color) {
  canvas.setFont(&fonts::FreeSansBold24pt7b);
  int n = strlen(name);
  int total = canvas.textWidth(name);
  int x = (W - total) / 2;
  canvas.setTextDatum(top_left);

  for (int gone = 1; gone <= n; gone++) {
    for (int step = 0; step < 4; step++) {
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
  uint8_t winner = (random(1000) < MARVIN_CHANCE * 1000) ? 1 : 2;
  bool overruled = false;
  if (winner == 2) {
    bool streak = NO_LEA_STREAK && lastWinner == 2;
    if (streak || random(1000) < OVERRULE_ODDS * 1000) { winner = 1; overruled = true; }
  }
  lastWinner = winner;
  bool correction = overruled || (winner == 1 && random(1000) < CORRECTION_ODDS * 1000);
  const char* overrule = correction ? OVERRULES[pickUnused(usedOver, COUNT(OVERRULES))] : nullptr;

  for (int i = 0; i < LINES_PER_ROUND; i++) {
    holdLine(LINES[pickUnused(usedLines, COUNT(LINES))], LINE_MS);
    thinkLine(THINKING[pickUnused(usedThink, COUNT(THINKING))], THINK_MS);
  }
  thinkLine(FINALIZING[pickUnused(usedFinal, COUNT(FINALIZING))], FINAL_MS);

  if (correction) {
    stampName("LEA", C_LEA);
    delay(900);
    dropName("LEA", C_LEA);
    holdLine(overrule, 2400, C_RED);
  }
  const char* name = (winner == 1) ? "MARVIN" : "LEA";
  stampName(name, winner == 1 ? C_MARVIN : C_LEA);
  M5.Speaker.tone(1800, 120);

  const char* reason = correction ? overrule
                     : (winner == 1) ? REASONS_MARVIN[pickUnused(usedReason, COUNT(REASONS_MARVIN))]
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
   5) POWER — sleep with the IMU left on, so a shake wakes it
   ========================================================== */
M5PM1  pm1;
BMI270 imu;
bool pm1_ok = false, imu_ok = false;

void powerSetup() {
  auto sda = M5.getPin(m5::pin_name_t::in_i2c_sda);
  auto scl = M5.getPin(m5::pin_name_t::in_i2c_scl);
  Wire.end();
  Wire.begin(sda, scl, 100000U);

  pm1_ok = (pm1.begin(&Wire, M5PM1_DEFAULT_ADDR, sda, scl, M5PM1_I2C_FREQ_100K) == M5PM1_OK);
  if (pm1_ok) {
    // IMU INT1 is wired to the PMIC's GPIO4; a falling edge there wakes the PMIC.
    pm1.gpioSetWakeEnable(M5PM1_GPIO_NUM_4, true);
    pm1.gpioSetWakeEdge(M5PM1_GPIO_NUM_4, M5PM1_GPIO_WAKE_FALLING);
  }
  imu_ok = (imu.beginI2C(BMI2_I2C_PRIM_ADDR) == BMI2_OK);
  if (imu_ok) imu.disableFeature(BMI2_ANY_MOTION);   // only armed right before sleep
}

void goToSleep() {
  frameBegin();
  drawWrapped("Shake to wake.", C_DIM, &fonts::FreeSans9pt7b, H / 2);
  frameEnd();
  delay(700);

  if (pm1_ok && imu_ok) {
    // Arm the IMU: "any motion" above the threshold pulls INT1 low.
    bmi2_sens_config cfg = {};
    cfg.type = BMI2_ANY_MOTION;
    cfg.cfg.any_motion.threshold = SHAKE_THRESHOLD;
    cfg.cfg.any_motion.duration  = SHAKE_DURATION;
    cfg.cfg.any_motion.select_x = 1;
    cfg.cfg.any_motion.select_y = 1;
    cfg.cfg.any_motion.select_z = 1;
    imu.setConfig(cfg);
    imu.enableFeature(BMI2_ANY_MOTION);

    bmi2_int_pin_config ip = {};
    ip.pin_type  = BMI2_INT1;
    ip.int_latch = BMI2_INT_NON_LATCH;
    ip.pin_cfg[0].lvl       = BMI2_INT_ACTIVE_LOW;
    ip.pin_cfg[0].od        = BMI2_INT_PUSH_PULL;
    ip.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;
    ip.pin_cfg[0].input_en  = BMI2_INT_INPUT_DISABLE;
    imu.setInterruptPinConfig(ip);
    imu.mapInterruptToPin(BMI2_ANY_MOTION_INT, BMI2_INT1);

    M5.Display.sleep();
    // Keep the IMU's rail (L1) alive while the PMIC sleeps, then sleep.
    pm1.setLdoEnable(true);
    pm1.ldoSetPowerHold(true);
    pm1.setLedEnLevel(true);
    pm1.shutdown();
  }
  // Fallback if the PMIC or IMU didn't come up: plain power off (power button wakes).
  M5.Display.sleep();
  M5.Power.powerOff();
  while (true) delay(1000);
}

/* ==========================================================
   6) SETUP + LOOP
   ========================================================== */
uint32_t lastPress = 0;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setBrightness(180);
  W = M5.Display.width(); H = M5.Display.height();
  canvas.createSprite(W, H);
  initColors();
  randomSeed(esp_random());
  loadState();
  powerSetup();
  showIdle();
  lastPress = millis();
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    lastPress = millis();
    decide();
    uint32_t t = millis();
    while (millis() - t < RESULT_MS) {
      M5.update();
      if (M5.BtnA.wasPressed() || M5.BtnB.wasPressed()) break;
      delay(10);
    }
    showIdle();
    lastPress = millis();
  }

  if (M5.BtnB.wasPressed()) {
    lastPress = millis();
    showTally();
    delay(2500);
    showIdle();
  }

  if (millis() - lastPress > IDLE_OFF_MS) goToSleep();
  delay(10);
}
