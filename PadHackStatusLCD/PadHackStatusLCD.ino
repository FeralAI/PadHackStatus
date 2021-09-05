#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef SEEED_XIAO_M0
#define REFERENCE_VOLTAGE 3.25f // Seeeduino XIAO seems to be .5V below typicaly 3.3V reference
#else
#define REFERENCE_VOLTAGE 3.3f
#endif

#define ANALOG_RANGE 1024

#define LED1_INPUT_PIN 0
#define LED2_INPUT_PIN 1
#define LED3_INPUT_PIN 2
#define LED4_INPUT_PIN 3
#define LED_COUNT 4
#define LED_RADIUS 8
#define LED_PADDING 11

#define BATT_PIN A10
#define BATT_PERCENT_DIVISIONS 5
#define BATT_MIN_MILLIVOLTS 3200
#define BATT_MAX_MILLIVOLTS 4200
#define BATT_MAX_HISTORY 5
#define BATT_POLL_INTERVAL_MS 2000

uint8_t inputLedPins[] = { LED1_INPUT_PIN, LED2_INPUT_PIN, LED3_INPUT_PIN, LED4_INPUT_PIN };

Adafruit_SSD1306 lcd = Adafruit_SSD1306(128, 32);
int vbattRaw;
float vbattAverage;
float vbattHistory[BATT_MAX_HISTORY];
uint8_t vbattPercent;
uint8_t historyIndex = 0;
uint64_t lastBattPollTime;

// LED state vars
uint8_t ledArrayState;
uint8_t lastLedArrayState;

void setup() {
  // Delay for LCD driver to initialize
  delay(125);

  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lcd.clearDisplay();
  lcd.display();
  lcd.setTextSize(1);
  lcd.setTextColor(SSD1306_WHITE);

  // Set up LED pins
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(inputLedPins[i], INPUT_PULLUP); // Status pin
  }

  pinMode(BATT_PIN, INPUT);

  // Perform initial battery reading
  readBattery();
}

void loop() {
  uint64_t battPollTime = millis() - lastBattPollTime;
  if (battPollTime < 0 || battPollTime >= BATT_POLL_INTERVAL_MS)
    readBattery();

  readLeds();
  updateDisplay();
}

void readBattery() {
  // Cache the previous value
  lastBattPollTime = millis();
  
  // Read the analog battery value
  vbattRaw = analogRead(BATT_PIN);

  // Convert analog reading to battery voltage
  float vbatt = vbattRaw;
  vbatt *= 2;                 // Multiply by # of resistors voltage is divided across
  vbatt *= 1000;              // Convert to mV for better precision
  vbatt *= REFERENCE_VOLTAGE; // Multiply by reference voltage
  vbatt /= ANALOG_RANGE;      // Convert analog reading to voltage

  // Persist vbatt value in history
  vbattHistory[historyIndex] = vbatt;
  historyIndex = (historyIndex >= BATT_MAX_HISTORY) ? 0 : (historyIndex + 1);

  // Get the running vbatt average
  float vbattTotal = 0;
  int vbattCount = 0;
  for (int i = 0; i < BATT_MAX_HISTORY; i++) {
    if (vbattHistory[i]) {
      vbattTotal += vbattHistory[i];
      vbattCount++;
    }
  }
  vbattAverage = vbattTotal / vbattCount;

  float percent = constrain(vbattAverage, BATT_MIN_MILLIVOLTS, BATT_MAX_MILLIVOLTS); // Limit to usable voltage range
  percent -= BATT_MIN_MILLIVOLTS;                                                    // Get the value above minimum
  percent /= 10;                                                                     // Convert to percentage
  vbattPercent = round(percent / BATT_PERCENT_DIVISIONS) * BATT_PERCENT_DIVISIONS;   // Apply percentage divisions
}

void readLeds() {
  lastLedArrayState = ledArrayState;
  for (int i = 0; i < LED_COUNT; i++) {
    // If HIGH, not on
    if (analogRead(inputLedPins[i]) > 900)
      ledArrayState &= ~(1 << i);
    else
      ledArrayState |= (1 << i);
  }
}

void updateDisplay() {
  lcd.clearDisplay();

  // // Draw battery gauge outline
  // lcd.drawFastHLine(0, 1, 101, SSD1306_WHITE);
  // lcd.drawFastHLine(0, 5, 101, SSD1306_WHITE);
  // lcd.drawFastVLine(0, 1, 5, SSD1306_WHITE);
  // lcd.drawFastVLine(101, 1, 5, SSD1306_WHITE);

  // // Fill battery gauge
  // for (int i = 2; i < 5; i++) {
  //   lcd.drawFastHLine(1, i, vbattPercent, SSD1306_WHITE);
  // }

  // Show voltage
  lcd.setCursor(0, 0);
  // lcd.print(vbattRaw);
  // lcd.setCursor(55, 0);
  lcd.print(vbattAverage / 1000, 2);
  lcd.print("V ");

  // Show percentage
  uint8_t cursorPosX = 110;
  if (vbattPercent < 10) {
    cursorPosX += 6;
  } else if (vbattPercent > 99) {
    cursorPosX -= 6;
  }
  lcd.setCursor(cursorPosX, 0);
  lcd.print(vbattPercent);
  lcd.print("%");

  // Draw LEDs 1-4
  for (int i = 0; i < 4; i++) {
    int x = i * (LED_RADIUS * 2);
    // If HIGH, on
    if ((ledArrayState >> i) & 1) {
      lcd.fillCircle(x + LED_RADIUS + (i * LED_PADDING), LED_RADIUS + 14, LED_RADIUS, SSD1306_WHITE);
    } else {
      lcd.drawCircle(x + LED_RADIUS + (i * LED_PADDING), LED_RADIUS + 14, LED_RADIUS, SSD1306_WHITE);
    }
  }

  // Draw Home LED (deprecated)
  int homeX = 4 * (LED_RADIUS * 2);
  if (homeLedState)
    lcd.fillCircle(homeX + LED_RADIUS + (4 * LED_PADDING), LED_RADIUS + 14, LED_RADIUS, SSD1306_WHITE);
  else
    lcd.drawCircle(homeX + LED_RADIUS + (4 * LED_PADDING), LED_RADIUS + 14, LED_RADIUS, SSD1306_WHITE);

  lcd.display();
}
