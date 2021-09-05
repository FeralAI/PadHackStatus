#ifdef SEEED_XIAO_M0
#define REFERENCE_VOLTAGE 3.25f // Seeeduino XIAO seems to be .05v below typical 3.3v reference
#else
#define REFERENCE_VOLTAGE 3.3f
#endif

#define ANALOG_RANGE 1024

#define LED1_INPUT_PIN 0
#define LED2_INPUT_PIN 1
#define LED3_INPUT_PIN 2
#define LED4_INPUT_PIN 3
#define LED1_OUTPUT_PIN 4
#define LED2_OUTPUT_PIN 5
#define LED3_OUTPUT_PIN 6
#define LED4_OUTPUT_PIN 7
#define LED_COUNT 4

#define BATT_LED_PIN 8
#define BATT_INPUT_PIN A10
#define BATT_LOW_MILLIVOLTS 3350
#define BATT_MAX_HISTORY 5
#define BATT_POLL_INTERVAL_MS 2000

uint8_t inputLedPins[] =  { LED1_INPUT_PIN,  LED2_INPUT_PIN,  LED3_INPUT_PIN,  LED4_INPUT_PIN };
uint8_t outputLedPins[] = { LED1_OUTPUT_PIN, LED2_OUTPUT_PIN, LED3_OUTPUT_PIN, LED4_OUTPUT_PIN };

// Battery state vars
uint16_t vbattRaw;
float vbattAverage;
float vbattHistory[BATT_MAX_HISTORY];
uint8_t historyIndex = 0;
uint64_t lastBattPollTime;

// LED state vars
uint8_t ledArrayState;
uint8_t lastLedArrayState;
bool battLedState;
bool lastBattLedState;

void setup() {
  // Set up LED pins
  for (int i = 0; i < LED_COUNT; i++) {
    pinMode(inputLedPins[i], INPUT_PULLUP);
    pinMode(outputLedPins[i], OUTPUT);
    digitalWrite(outputLedPins[i], LOW);
  }

  // Set up battery
  pinMode(BATT_INPUT_PIN, INPUT);
  pinMode(BATT_LED_PIN, OUTPUT);
  digitalWrite(BATT_LED_PIN, LOW);

  // Perform initial battery reading
  readBattery();
}

void loop() {
  uint64_t battPollTime = millis() - lastBattPollTime;
  if (battPollTime >= BATT_POLL_INTERVAL_MS)
    readBattery();

  readLeds();
  updateDisplay();
}

void readBattery() {
  // Cache the previous values
  lastBattPollTime = millis();
  lastBattLedState = battLedState;

  // Read the analog battery value
  vbattRaw = analogRead(BATT_INPUT_PIN);

  // Convert analog reading to battery voltage
  float vbatt = vbattRaw;
  vbatt *= 2;                 // Multiply by # of resistors voltage is divided across
  vbatt *= 1000;              // Convert to mV for better precision
  vbatt *= REFERENCE_VOLTAGE; // Multiply by reference voltage
  vbatt /= ANALOG_RANGE;      // Convert analog reading to mV

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

  // Set the battery LED state
  battLedState = vbattAverage < BATT_LOW_MILLIVOLTS;
}

void readLeds() {
  // Cache the previous values
  lastLedArrayState = ledArrayState;

  // Read the LED input pins
  for (int i = 0; i < LED_COUNT; i++) {
    int ledValue = analogRead(inputLedPins[i]);

    // If HIGH, not on
    if (ledValue > 900)
      ledArrayState &= ~(1 << i);
    else
      ledArrayState |= (1 << i);
  }
}

void updateDisplay() {
  if (lastLedArrayState != ledArrayState) {
    for (int i = 0; i < LED_COUNT; i++) {
      uint8_t ledState = (ledArrayState >> i) & 1;
      if (ledState != ((lastLedArrayState >> i) & 1))
        digitalWrite(outputLedPins[i], ledState);
    }
  }

  if (lastBattLedState != battLedState)
    digitalWrite(BATT_LED_PIN, battLedState);
}
