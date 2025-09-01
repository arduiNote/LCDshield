#include <LiquidCrystal.h>

// ---------------- Types first (avoid Arduino preprocessor issues) ----------------
struct Obstacle {
  int col;
  int row;   // 0 = top, 1 = bottom
  bool active;
};

// ---------------- Prototypes ----------------
enum Btn { BTN_NONE, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT };
Btn  readButton();
Btn  readButtonDebounced(unsigned long debounceMs = 20);
void spawnObstacle(Obstacle &o);
void drawRunner(int playerRow, int obsCol, int obsRow, bool obsActive, unsigned long score);
void gameRunner();

// ---------------- LCD ----------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ---------------- Buttons ----------------
Btn readButton() {
  int x = analogRead(A0);   // LCD Keypad Shield ladder on A0
  if (x < 60)   return BTN_RIGHT;
  if (x < 200)  return BTN_UP;
  if (x < 400)  return BTN_DOWN;
  if (x < 600)  return BTN_LEFT;
  if (x < 800)  return BTN_SELECT;
  return BTN_NONE;
}

Btn readButtonDebounced(unsigned long debounceMs) {
  Btn b1 = readButton();
  delay(debounceMs);
  Btn b2 = readButton();
  return (b1 == b2) ? b1 : BTN_NONE;
}

// ---------------- Game helpers ----------------
void spawnObstacle(Obstacle &o) {
  o.col = 15;
  o.row = random(2); // 0 or 1
  o.active = true;
}

void drawRunner(int playerRow, int obsCol, int obsRow, bool obsActive, unsigned long score) {
  lcd.clear();
  // row 0
  lcd.setCursor(0,0);
  for (int c=0; c<16; c++) {
    if (obsActive && obsRow==0 && c==obsCol) lcd.print('|');
    else if (playerRow==0 && c==0)          lcd.print('O');
    else                                    lcd.print(' ');
  }
  // row 1
  lcd.setCursor(0,1);
  for (int c=0; c<16; c++) {
    if (obsActive && obsRow==1 && c==obsCol) lcd.print('|');
    else if (playerRow==1 && c==0)           lcd.print('O');
    else                                     lcd.print(' ');
  }
  // score (top-right)
  lcd.setCursor(13,0);
  char buf[4];
  snprintf(buf, sizeof(buf), "%2lu", score % 100);
  lcd.print(buf);
}

// ---------------- Game loop ----------------
void gameRunner() {
  randomSeed(analogRead(A1)); // any floating analog pin

  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Runner: UP=jump");
  lcd.setCursor(0,1); lcd.print("LEFT=exit");
  delay(900);

  int playerRow = 1;               // start bottom
  int jumpTicks = 0;               // >0 => on top row
  const int jumpDuration = 5;

  Obstacle o{ -1, 0, false };
  unsigned long lastStep = 0;
  unsigned long stepDelay = 140;   // ms per step, speeds up
  unsigned long score = 0;
  unsigned long lastSpawn = 0;
  unsigned long spawnInterval = 900; // ms, decreases

  while (true) {
    // Input
    Btn b = readButtonDebounced();
    if (b == BTN_UP && jumpTicks == 0) { jumpTicks = jumpDuration; playerRow = 0; }
    if (b == BTN_DOWN) { jumpTicks = 0; playerRow = 1; }
    if (b == BTN_LEFT) { lcd.clear(); return; }

    unsigned long now = millis();

    // Spawn
    if (!o.active && (now - lastSpawn >= spawnInterval)) {
      spawnObstacle(o);
      lastSpawn = now;
      if (spawnInterval > 450) spawnInterval -= 8;
    }

    // Tick
    if (now - lastStep >= stepDelay) {
      lastStep = now;

      if (o.active) {
        o.col--;
        if (o.col < 0) {
          o.active = false;
          score++;
          if (stepDelay > 80) stepDelay -= 2;
        }
      }

      if (jumpTicks > 0) {
        jumpTicks--;
        if (jumpTicks == 0) playerRow = 1; // fall
      }

      // Collision (player at col 0)
      if (o.active && o.col == 0 && o.row == playerRow) {
        lcd.clear();
        lcd.setCursor(0,0); lcd.print("Crash! Score:");
        lcd.setCursor(0,1); lcd.print(score); lcd.print(" SEL=again");
        // wait for choice
        while (true) {
          Btn b2 = readButtonDebounced();
          if (b2 == BTN_SELECT) { gameRunner(); return; }
          if (b2 == BTN_LEFT || b2 == BTN_RIGHT) return;
        }
      }

      drawRunner(playerRow, o.col, o.row, o.active, score);
    }
  }
}

// ---------------- Arduino setup/loop ----------------
void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Obstacle Runner");
  lcd.setCursor(0,1); lcd.print("Press SELECT...");
  while (readButton() != BTN_SELECT) { /* wait */ }
}

void loop() {
  gameRunner();
  // after exit, wait to restart
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("SELECT: play");
  lcd.setCursor(0,1); lcd.print("LEFT:   quit");
  unsigned long t0 = millis();
  while (millis() - t0 < 15000) {
    Btn b = readButtonDebounced();
    if (b == BTN_SELECT) return;     // play again
    if (b == BTN_LEFT) { lcd.clear(); while(true){} } // stop here
  }
}
