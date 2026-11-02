/*
  Touch Light Project — Board 0008
  Seeed XIAO ESP32-C3 on Rev1H board 0008 (GPIO21 touch, GPIO4 PWM)
  Software Rev V1.2.6

  Touch input, LED PWM output and heartbeat GPIO are configured in BoardConfig.h.
  GPIO connections verified from PCB routing and functional touch/PWM tests.

  Operation:
    Short touch (< 1 second): Toggle light ON/OFF
    Long touch (> 1 second): Continuously dim up/down while held
    Release: Stops at the selected brightness

  Universal firmware:
    iPhone browser control + optional MQTT Home Assistant discovery,
    both over the same Wi-Fi. Local touch always works without Wi-Fi.
*/

#include <Arduino.h>
#include "WebControl.h"
#include "OptionalMqtt.h"
#include "Diagnostics.h"
#include "ResetMonitor.h"

// GPIO mapping for the verified Rev1H board 0008.
#include "BoardConfig.h"

// Touch configuration
const bool TOUCH_ACTIVE_STATE = LOW;

// PWM configuration
const uint32_t PWM_FREQUENCY = 1000;
const uint8_t PWM_RESOLUTION = 8;
const uint8_t MIN_BRIGHTNESS = 5;
const uint8_t MAX_BRIGHTNESS = 255;

// Timing
const unsigned long LONG_PRESS_TIME = 1000;
const unsigned long RAMP_INTERVAL = 20;
const unsigned long DEBOUNCE_TIME = 75;
unsigned long lastTouchChangeTime = 0;

// Light state
bool lightOn = false;
uint8_t brightness = 180;
uint8_t savedBrightness = 180;

// Touch state
bool previousTouchState = false;
unsigned long touchStartTime = 0;
bool longPressActive = false;

// Dimming state
bool rampingUp = false;
unsigned long lastRampTime = 0;

// PWM is eight-bit: each count represents 1/255 of the full duty cycle.
// Rounded to the nearest whole percent for readability.
unsigned pwmDutyPercent(uint8_t level)
{
  return (static_cast<unsigned>(level) * 100U + 127U) / 255U;
}

// Diagnostic log: keep the engineering PWM count *and* show a user-friendly
// duty-cycle percentage. This is not a calibrated measurement of LED light.
void logLightChange(const char *source)
{
  const unsigned level = static_cast<unsigned>(brightness);
  const unsigned pct = pwmDutyPercent(brightness);

  if (lightOn)
    diagLogf("[%s] Light ON, brightness %u/255 (%u%% PWM)",
             source, level, pct);
  else
    diagLogf("[%s] Light OFF, saved %u/255 (%u%%); output 0%%",
             source, level, pct);
}

void setBrightness(uint8_t level)
{
  brightness = constrain(level, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
  ledcWrite(LED_PWM_PIN, lightOn ? brightness : 0);
}

void webControlGetState(bool &powerOn, uint8_t &level)
{
  powerOn = lightOn;
  level = brightness;
}

void toggleLight()
{
  if (lightOn)
  {
    savedBrightness = brightness;
    lightOn = false;
    ledcWrite(LED_PWM_PIN, 0);
    Serial.println("LIGHT OFF");
  }
  else
  {
    lightOn = true;
    brightness = savedBrightness;
    ledcWrite(LED_PWM_PIN, brightness);
    Serial.print("LIGHT ON - Brightness = ");
    Serial.println(brightness);
  }

  logLightChange("TOUCH");
  optionalMqttPublishState(lightOn,brightness);
}

void rampBrightness()
{
  unsigned long now = millis();

  if (now - lastRampTime < RAMP_INTERVAL)
    return;

  lastRampTime = now;

  if (rampingUp)
  {
    if (brightness < MAX_BRIGHTNESS)
      brightness++;
    else
      rampingUp = false;
  }
  else
  {
    if (brightness > MIN_BRIGHTNESS)
      brightness--;
    else
      rampingUp = true;
  }

  ledcWrite(LED_PWM_PIN, brightness);
}

// Common control application for WEB and HOME ASSISTANT command sources.
void applyRemoteCommand(
  bool powerCommandValid,
  bool powerOn,
  bool brightnessCommandValid,
  uint8_t requestedBrightness,
  const char *source)
{
  // Home Assistant may send both an ON command and a brightness command
  // for one slider adjustment. Their combined effect can include a
  // second command that makes no change to the lamp. Capture the old
  // state so duplicate/no-op commands do not clutter the event log or
  // republish unchanged state to MQTT.
  const bool wasOn = lightOn;
  const uint8_t wasBrightness = brightness;
  const uint8_t wasSavedBrightness = savedBrightness;

  if (brightnessCommandValid)
  {
    if (requestedBrightness == 0)
    {
      if (lightOn)
        savedBrightness = brightness;

      lightOn = false;
      ledcWrite(LED_PWM_PIN, 0);
    }
    else
    {
      brightness = constrain(requestedBrightness, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
      savedBrightness = brightness;

      if (lightOn)
        ledcWrite(LED_PWM_PIN, brightness);
    }
  }

  if (powerCommandValid)
  {
    if (powerOn)
    {
      lightOn = true;
      brightness = savedBrightness;
      ledcWrite(LED_PWM_PIN, brightness);
    }
    else
    {
      if (lightOn)
        savedBrightness = brightness;

      lightOn = false;
      ledcWrite(LED_PWM_PIN, 0);
    }
  }

  // Don't generate another [HOME ASSISTANT] event when the device is
  // already in the requested state. Applies equally to redundant WEB
  // commands. Real transitions and brightness adjustments are logged.
  if (lightOn == wasOn &&
      brightness == wasBrightness &&
      savedBrightness == wasSavedBrightness)
    return;

  Serial.print("["); Serial.print(source); Serial.print("] Light = ");
  Serial.print(lightOn ? "ON" : "OFF");
  Serial.print("  Brightness = ");
  Serial.println(brightness);

  logLightChange(source);
  optionalMqttPublishState(lightOn,brightness);
}

void webControlCommand(bool powerValid,bool powerOn,bool brightnessValid,uint8_t level,const char *clientName)
{
  String source="WEB";
  if(clientName && clientName[0]) {source+=": ";source+=clientName;}
  applyRemoteCommand(powerValid,powerOn,brightnessValid,level,source.c_str());
}

void mqttControlCommand(bool powerValid,bool powerOn,bool brightnessValid,uint8_t level)
{
  applyRemoteCommand(powerValid,powerOn,brightnessValid,level,"HOME ASSISTANT");
}

void updateHeartbeatLED()
{
  const unsigned long HEARTBEAT_PERIOD_MS = 1000;
  const unsigned long FLASH_DURATION_MS = 100;
  unsigned long cyclePosition = millis() % HEARTBEAT_PERIOD_MS;

  if (STATUS_LED_PRESENT)
  {
    digitalWrite(
      STATUS_LED_PIN,
      cyclePosition < FLASH_DURATION_MS ? LOW : HIGH
    );
  }
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  if (STATUS_LED_PRESENT)
  {
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH);
  }

  Serial.println();
  Serial.println("-----------------------------");
  Serial.println("TECH 49 LAB - Touch Light");
  Serial.printf("Touch GPIO%u / PWM GPIO%u\n", TOUCH_PIN, LED_PWM_PIN);
  Serial.println("-----------------------------");

  // Capture ESP-IDF reset reason and update small nonvolatile reset counters.
  // Must run before web setup so reason is visible in diagnostics immediately.
  resetMonitorBegin();

  // Board 0008 touch IC is IQS228B; active-low behaviour verified on bench.
  pinMode(TOUCH_PIN, TOUCH_INPUT_MODE);

  bool pwmOK = ledcAttach(LED_PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

  if (pwmOK) {
    Serial.println("PWM initialized.");
    diagLog("[SYSTEM] LED PWM initialized on GPIO4");
  } else {
    Serial.println("ERROR: PWM initialization failed.");
    diagLog("[SYSTEM] ERROR: LED PWM initialization failed");
  }

  ledcWrite(LED_PWM_PIN, 0);

  webControlBegin();
  optionalMqttPublishState(lightOn,brightness);

  // Arm only after setup/provisioning has completed. Keep a generous 60 s
  // watchdog period to avoid false resets during normal network operations.
  resetMonitorArmWatchdog();

  Serial.println("System ready.");
}

void loop()
{
  updateHeartbeatLED();
  webControlLoop();

  unsigned long now = millis();
  bool rawTouchState = (digitalRead(TOUCH_PIN) == TOUCH_ACTIVE_STATE);

  static bool lastRawTouchState = false;
  static bool debouncedTouchState = false;

  if (rawTouchState != lastRawTouchState)
  {
    lastTouchChangeTime = now;
    lastRawTouchState = rawTouchState;
  }

  if (now - lastTouchChangeTime >= DEBOUNCE_TIME)
    debouncedTouchState = rawTouchState;

  bool touchState = debouncedTouchState;

  // When viewing the individual setup password, a deliberate 4-second
  // physical hold authorizes the browser. Do not also toggle or dim the LED.
  if (diagInterceptTouch(touchState,now)) {
    previousTouchState=false;
    longPressActive=false;
    resetMonitorFeedWatchdog();
    return;
  }

  if (touchState && !previousTouchState)
  {
    touchStartTime = now;
    longPressActive = false;
    Serial.println("Touch detected.");
    diagLog("[TOUCH] Touch detected");
  }

  if (touchState)
  {
    unsigned long touchDuration = now - touchStartTime;

    if (!longPressActive && touchDuration >= LONG_PRESS_TIME)
    {
      longPressActive = true;
      Serial.println("Long press - dimming.");
      diagLog("[TOUCH] Long press: dimming");

      if (!lightOn)
      {
        lightOn = true;
        brightness = savedBrightness;
        ledcWrite(LED_PWM_PIN, brightness);
      }
    }

    if (longPressActive)
      rampBrightness();
  }

  if (!touchState && previousTouchState)
  {
    unsigned long touchDuration = now - touchStartTime;

    if (!longPressActive && touchDuration < LONG_PRESS_TIME)
    {
      toggleLight();
    }
    else
    {
      savedBrightness = brightness;
      Serial.print("Dimming stopped - Brightness = ");
      Serial.println(brightness);
      diagLogf("[TOUCH] Light ON, dimming stopped, brightness %u/255 (%u%% PWM)",
               static_cast<unsigned>(brightness),
               pwmDutyPercent(brightness));
      optionalMqttPublishState(lightOn,brightness);
    }

    longPressActive = false;
  }

  previousTouchState = touchState;
  resetMonitorFeedWatchdog();
}

