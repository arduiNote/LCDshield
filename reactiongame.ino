#include <LiquidCrystal.h>

/*
  LCD Keypad Shield — Reaction Test
  - The display shows a prompt like: "PRESS: UP"
  - You have limited time to press the matching button.
  - Correct = +1 score and the time window shrinks (harder).
  - Wrong / too slow = lose 1 life. 3 lives total.
  - Press SELECT on the "Game Over" screen to play again.
*/

// ---------------- LCD wiring (common for most shields) ----------------
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// ---------------- Button handling ----------------
enum Btn { BTN_NONE, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_SELECT };

Btn readButtonRaw() {
  int x = analogRead(A0);  // resistor ladder on A0
  if (x < 60)   return BTN_RIGHT;
  if (x < 200)  return BTN_UP;
  if (x < 400)  return BTN_DOWN;
  if (x < 600)  return BTN_LEFT;
  if (x < 800)  return BTN_SELECT;
  return BTN_NONE;
}

Btn readButtonDebounced(unsigned long debounceMs = 20) {
  Btn b1 = readButtonRaw();
  delay(debounceMs);
  Btn b2 = readButtonRaw();
  return (b1 == b2) ? b1 : BTN_NONE;
}

const char* btnName(Btn b) {
  switch (b) {
    case BTN_RIGHT:  return "RIGHT";
    case BTN_UP:     return "UP";
    case BTN_DOWN:   return "DOWN";
    case BTN_LEFT:   return "LEFT";
    case BTN_SELECT: return "SELECT";
    default:         return "NONE";
  }
}

// Wait for any button press (debounced) with timeout (ms).
// Returns BTN_NONE if no press in time.
Btn waitForPressWithTimeout(unsigned long timeoutMs) {
  unsigned long t0 = millis();
  // Ensure we don't read a held button from previous round:
  while (readButtonRaw() != BTN_NONE) { /* wait release */ }
  while (millis() - t0 < timeoutMs) {
    Btn b = readButtonDebounced();
    if (b != BTN_NONE) return b;
  }
  return BTN_NONE;
}

// ---------------- Game ----------------
void showIntro() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Reaction Test");
  lcd.setCursor(0,1); lcd.print("SELECT to start");
  // wait for SELECT
  while (readButtonDebounced() != BTN_SELECT) { /* idle */ }
  delay(200);
}

Btn randomTarget() {
  // Choose among the 5 physical buttons
  int r = random(5);
  switch (r) {
    case 0: return BTN_UP;
    case 1: return BTN_DOWN;
    case 2: return BTN_LEFT;
    case 3: return BTN_RIGHT;
    default: return BTN_SELECT;
  }
}

void showPrompt(Btn target, int score, int lives, unsigned long windowMs) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PRESS: ");
  lcd.print(btnName(target));
  lcd.setCursor(0,1);
  // Show score and lives and window (quick HUD)
  // e.g. S:12 L:3 T:900
  lcd.print("S:");
  lcd.print(score);
  lcd.print(" L:");
  lcd.print(lives);
  lcd.print(" T:");
  lcd.print(windowMs);
}

void flashMessage(const char* line1, const char* line2, unsigned long ms = 700) {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(line1);
  lcd.setCursor(0,1); lcd.print(line2);
  delay(ms);
}

void gameLoop() {
  int score = 0;
  int lives = 3;

  // Timing window starts generous, shrinks on correct answers.
  unsigned long windowMs = 1200;     // start reaction window
  const unsigned long minWindowMs = 450;
  const unsigned long shrinkStep   = 40; // shrink per correct
  const unsigned long wrongPenalty = 0;  // optionally increase window on wrong

  randomSeed(analogRead(A1)); // any floating pin

  while (lives > 0) {
    Btn target = randomTarget();
    showPrompt(target, score, lives, windowMs);

    Btn pressed = waitForPressWithTimeout(windowMs);

    if (pressed == target) {
      score++;
      flashMessage("Correct!", " +1 score", 400);
      if (windowMs > minWindowMs + shrinkStep) windowMs -= shrinkStep;
      else windowMs = minWindowMs;
    } else if (pressed == BTN_NONE) {
      lives--;
      flashMessage("Too slow!", " -1 life", 600);
      if (wrongPenalty > 0) windowMs += wrongPenalty;
    } else {
      lives--;
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Wrong! Needed:");
      lcd.setCursor(0,1); lcd.print(btnName(target)); lcd.print("  -1 life");
      delay(800);
      if (wrongPenalty > 0) windowMs += wrongPenalty;
    }
  }

  // Game over screen
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Game Over! S:");
  lcd.print(score);
  lcd.setCursor(0,1); lcd.print("SEL=again  LEFT=quit");

  // Wait for decision
  while (true) {
    Btn b = readButtonDebounced();
    if (b == BTN_SELECT) { delay(150); return; } // play again (return to loop())
    if (b == BTN_LEFT)   { // quit to idle
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Thanks for play!");
      delay(800);
      // idle splash
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("Reaction Test");
      lcd.setCursor(0,1); lcd.print("SELECT to start");
      // wait for SELECT to start again
      while (readButtonDebounced() != BTN_SELECT) { /* idle */ }
      delay(150);
      return;
    }
  }
}

// ---------------- Arduino setup/loop ----------------
void setup() {
  lcd.begin(16, 2);
  showIntro();
}

void loop() {
  gameLoop();
}
