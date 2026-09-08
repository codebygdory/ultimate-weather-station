#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define TRIG_PIN 9
#define ECHO_PIN 10
#define ACTIVE_BUZZER 7
#define PASSIVE_BUZZER 8
#define LED_PIN 6
#define POT_PIN A1

// Notes for SOS and siren
#define NOTE_A4 440
#define NOTE_E5 659
#define NOTE_C5 523

// Sensitivity modes
const char* modeNames[] = {"HOME   ", "AWAY   ", "NIGHT  "};
int modeDistances[] = {40, 80, 20}; // trigger distances per mode
int currentMode = 0;

// State tracking
int alertLevel = 0;
unsigned long countdownStart = 0;
bool countdownActive = false;
bool alarmTriggered = false;
int alertCount = 0;
unsigned long tripwireStart = 0;
bool tripwireActive = false;

// Password reset - press pot all the way left 3 times
int potClickCount = 0;
bool lastPotLeft = false;
unsigned long lastPotClick = 0;

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void playSOSMorse() {
  // S = 3 short
  for (int i = 0; i < 3; i++) {
    tone(PASSIVE_BUZZER, NOTE_A4, 100);
    delay(200);
  }
  delay(200);
  // O = 3 long
  for (int i = 0; i < 3; i++) {
    tone(PASSIVE_BUZZER, NOTE_A4, 300);
    delay(400);
  }
  delay(200);
  // S = 3 short
  for (int i = 0; i < 3; i++) {
    tone(PASSIVE_BUZZER, NOTE_A4, 100);
    delay(200);
  }
  noTone(PASSIVE_BUZZER);
}

void playSiren() {
  tone(PASSIVE_BUZZER, NOTE_A4, 100);
  delay(100);
  tone(PASSIVE_BUZZER, NOTE_E5, 100);
  delay(100);
  noTone(PASSIVE_BUZZER);
}

void playWarning() {
  tone(PASSIVE_BUZZER, NOTE_C5, 200);
  delay(200);
  noTone(PASSIVE_BUZZER);
}

void checkPasswordReset() {
  int potValue = analogRead(POT_PIN);
  bool potLeft = (potValue < 50);
  
  if (potLeft && !lastPotLeft) {
    unsigned long now = millis();
    if (now - lastPotClick < 2000) {
      potClickCount++;
    } else {
      potClickCount = 1;
    }
    lastPotClick = now;
    
    if (potClickCount >= 3) {
      // Password accepted - reset alarm
      alarmTriggered = false;
      countdownActive = false;
      tripwireActive = false;
      alertLevel = 0;
      potClickCount = 0;
      digitalWrite(ACTIVE_BUZZER, LOW);
      digitalWrite(LED_PIN, LOW);
      noTone(PASSIVE_BUZZER);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Password Accept!");
      lcd.setCursor(0, 1);
      lcd.print("System Reset!   ");
      delay(2000);
      lcd.clear();
    }
  }
  lastPotLeft = potLeft;
}

void changeSensitivityMode() {
  int potValue = analogRead(POT_PIN);
  // Long hold pot all the way right to change mode
  if (potValue > 990) {
    delay(2000);
    potValue = analogRead(POT_PIN);
    if (potValue > 990) {
      currentMode = (currentMode + 1) % 3;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mode Changed!   ");
      lcd.setCursor(0, 1);
      lcd.print(modeNames[currentMode]);
      delay(2000);
      lcd.clear();
    }
  }
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ACTIVE_BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Security System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing... ");
  delay(2000);
  lcd.clear();
}

void loop() {
  checkPasswordReset();
  
  if (!alarmTriggered) {
    changeSensitivityMode();
  }

  int triggerDistance = modeDistances[currentMode];
  float distance = getDistance();

  // Determine zones
  if (distance >= 999 || distance > triggerDistance) {
    alertLevel = 0;
    tripwireActive = false;
    tripwireStart = 0;
  } else if (distance > triggerDistance * 0.66) {
    alertLevel = 1; // Zone 1 - Warning
  } else if (distance > triggerDistance * 0.33) {
    alertLevel = 2; // Zone 2 - Danger
  } else {
    alertLevel = 3; // Zone 3 - Critical
  }

  // Laser tripwire - must stay in range for 3 seconds
  if (alertLevel > 0) {
    if (!tripwireActive) {
      tripwireActive = true;
      tripwireStart = millis();
    }
    
    unsigned long tripwireElapsed = millis() - tripwireStart;
    
    if (tripwireElapsed >= 3000 && !countdownActive && !alarmTriggered) {
      countdownActive = true;
      countdownStart = millis();
      alertCount++;
    }
  }

  // Reset if clear
  if (alertLevel == 0 && !alarmTriggered) {
    countdownActive = false;
    digitalWrite(ACTIVE_BUZZER, LOW);
    digitalWrite(LED_PIN, LOW);
    noTone(PASSIVE_BUZZER);
  }

  // Countdown
  if (countdownActive && !alarmTriggered) {
    int elapsed = (millis() - countdownStart) / 1000;
    int remaining = 5 - elapsed;

    lcd.setCursor(0, 0);
    lcd.print("MOTION DETECTED!");
    lcd.setCursor(0, 1);
    lcd.print("Alarm in: ");
    lcd.print(remaining);
    lcd.print("s      ");

    if (remaining <= 0) {
      alarmTriggered = true;
      countdownActive = false;
    }

    playWarning();
    delay(500);
    return;
  }

  // Active alarm - SOS then siren
  if (alarmTriggered) {
    lcd.setCursor(0, 0);
    lcd.print("!! INTRUDER !!  ");
    lcd.setCursor(0, 1);
    lcd.print("Alerts:#");
    lcd.print(alertCount);
    lcd.print("       ");

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(ACTIVE_BUZZER, HIGH);
    playSOSMorse();
    digitalWrite(ACTIVE_BUZZER, LOW);
    playSiren();

    // Auto reset after 30 seconds
    static unsigned long alarmStart = 0;
    if (alarmStart == 0) alarmStart = millis();
    if (millis() - alarmStart >= 30000) {
      alarmTriggered = false;
      alarmStart = 0;
      digitalWrite(LED_PIN, LOW);
      noTone(PASSIVE_BUZZER);
    }
    return;
  }

  // Normal display
  lcd.setCursor(0, 0);
  switch (alertLevel) {
    case 0:
      lcd.print("CLEAR Mode:");
      lcd.print(modeNames[currentMode]);
      break;
    case 1:
      lcd.print("!! WARNING !!   ");
      break;
    case 2:
      lcd.print("!! DANGER !!    ");
      break;
    case 3:
      lcd.print("!! CRITICAL !!  ");
      break;
  }

  // Visual distance bar
  lcd.setCursor(0, 1);
  if (distance >= 999) {
    lcd.print("Dist: ------    ");
  } else {
    lcd.print("D:");
    lcd.print((int)distance);
    lcd.print("cm [");
    int barLength = map(constrain((int)distance, 0, triggerDistance), 0, triggerDistance, 8, 0);
    for (int i = 0; i < 8; i++) {
      if (i < barLength) lcd.print("#");
      else lcd.print("-");
    }
    lcd.print("]");
  }

  // Alert level behavior
  switch (alertLevel) {
    case 0:
      digitalWrite(LED_PIN, LOW);
      digitalWrite(ACTIVE_BUZZER, LOW);
      break;
    case 1:
      digitalWrite(LED_PIN, HIGH);
      delay(800);
      digitalWrite(LED_PIN, LOW);
      delay(200);
      break;
    case 2:
      digitalWrite(LED_PIN, HIGH);
      delay(400);
      digitalWrite(LED_PIN, LOW);
      delay(400);
      digitalWrite(ACTIVE_BUZZER, HIGH);
      delay(100);
      digitalWrite(ACTIVE_BUZZER, LOW);
      break;
    case 3:
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(ACTIVE_BUZZER, HIGH);
      delay(200);
      digitalWrite(ACTIVE_BUZZER, LOW);
      delay(200);
      break;
  }

  delay(100);
}