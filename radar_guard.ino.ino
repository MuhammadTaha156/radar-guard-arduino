#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Servo and ultrasonic sensor
Servo radarServo;
const int trigPin     = 11;
const int echoPin     = 10;
const int servoPin    = 12;

// LEDs and passive buzzer (controlled by tone()/noTone())
const int redLed      = 3;
const int greenLed    = 4;
const int buzzer      = 2;

// I2C LCD (address 0x27, 16x2 characters)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Configuration parameters
const int alertDistance         = 30;     // distance threshold for alert (cm)
const unsigned long updateInterval  = 20;     // servo scan interval (ms)
const int triggerThreshold      = 2;      // number of consecutive detections needed
const unsigned long displayDuration   = 500;    // LCD display time (ms)
const unsigned long blinkInterval     = 200;    // LED blink interval (ms)

// Scanning state variables (GLOBAL SCOPE)
int currentAngle      = 0;
int increment         = 1;
unsigned long lastServoUpdate = 0;

// Alert control variables (GLOBAL SCOPE)
bool displayAlarm       = false;    // whether showing text on LCD
bool alarmActive       = false;    // whether in blink alert state
unsigned long displayStartTime = 0;
unsigned long lastBlinkTime     = 0;
bool blinkState        = false;

// AI Integration Variables (GLOBAL SCOPE)
static int previousDistance = -1;
static unsigned long lastMovementTime = 0;
static int movementDetectedCount = 0;
const unsigned long movementThresholdTime = 300; // Adjust as needed (milliseconds)
const int distanceThreshold = 3;             // Adjust as needed (cm)
const int frequentMovementCount = 2;         // Adjust as needed (number of movements)

// Function Declarations (GLOBAL SCOPE)
float getDistance();
void startAlarmDisplay();
void alarmBlink();
void exitAlarm();
void lcdSetEmpty();

void setup() {
  radarServo.attach(servoPin);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  // buzzer uses tone()/noTone(), no pinMode needed

  lcd.init();
  lcd.backlight();
  lcd.clear();

  // show idle state
  lcdSetEmpty();
  digitalWrite(greenLed, HIGH);
}

void loop() {
  unsigned long now = millis();

  // 1. Smooth scanning & distance measurement
  if (now - lastServoUpdate >= updateInterval) {
    lastServoUpdate = now;
    radarServo.write(currentAngle);

    if (currentAngle % 5 == 0) {
      float d = getDistance();

      // AI Integration for Frequent Movement Detection
      bool currentMovement = false;
      if (previousDistance != -1 && abs((int)d - previousDistance) > distanceThreshold) {
        currentMovement = true;
        lastMovementTime = now;
      }
      previousDistance = (int)d;

      if (currentMovement && d < alertDistance) { // Only count movement if within alert range
        movementDetectedCount++;
      }

      if (now - lastMovementTime > movementThresholdTime) {
        movementDetectedCount = 0; // Reset count if no movement for a while
      }

      // Trigger alarm ONLY if frequent movement is detected within the alert distance
      if (movementDetectedCount >= frequentMovementCount && d < alertDistance && !alarmActive) {
        startAlarmDisplay(); // Trigger the alarm sequence
        movementDetectedCount = 0; // Reset count after triggering
      } else if (alarmActive && d >= alertDistance) {
        exitAlarm();
        movementDetectedCount = 0; // Reset count when exiting alarm
      }
      // End of AI Integration
    }

    // update servo angle back and forth between 0°–180°
    currentAngle += increment;
    if (currentAngle >= 180) {
      currentAngle = 180; increment = -1;
    } else if (currentAngle <= 0) {
      currentAngle = 0;  increment = 1;
    }
  }

  // 2. Manage LCD display duration & transition to blinking (remains the same)
  if (displayAlarm) {
    if (now - displayStartTime >= displayDuration) {
      displayAlarm = false;
      alarmActive  = true;
      lastBlinkTime = now;
    }
  }
  else if (alarmActive) {
    alarmBlink();
  }
}

// function to get distance
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

// function to display alarm
void startAlarmDisplay() {
  displayAlarm      = true;
  alarmActive       = false;
  displayStartTime = millis();

  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, LOW);
  noTone(buzzer);

  lcd.clear();
  char buf[6];
  sprintf(buf, "%d", currentAngle); // Using currentAngle for display
  int len1 = strlen(buf) + 1;
  int start1 = (16 - len1) / 2;
  lcd.setCursor(start1, 0);
  lcd.print(buf);
  lcd.write(223);

  const char* msg = "Foreign Body";
  int len2 = strlen(msg);
  int start2 = (16 - len2) / 2;
  lcd.setCursor(start2, 1);
  lcd.print(msg);
}

// function for alarm blink
void alarmBlink() {
  unsigned long now = millis();
  if (now - lastBlinkTime >= blinkInterval) {
    lastBlinkTime = now;
    blinkState = !blinkState;
    digitalWrite(redLed, blinkState);
    if (blinkState) tone(buzzer, 1000);
    else            noTone(buzzer);
  }
}

// function to exit alarm
void exitAlarm() {
  displayAlarm = false;
  alarmActive  = false;
  digitalWrite(redLed, LOW);
  noTone(buzzer);
  digitalWrite(greenLed, HIGH);
  lcdSetEmpty();
}

// function to set lcd empty
void lcdSetEmpty() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Area is Empty");
}