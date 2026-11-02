#pragma once
#include <Arduino.h>

// Stores recent system events in RAM (no password or MQTT credential logging).
// All callers are on the Arduino setup()/loop() thread; Wi-Fi event callback
// only caches its disconnect reason for subsequent loop-side logging.
void diagLog(const char *message);
void diagLogf(const char *format, ...);
String diagEventsJson();
// Clear only the 36-entry RAM event history; no firmware/Wi-Fi/MQTT reset.
void diagClearEvents();

// A one-time, short-lived physical confirmation is required to reveal the
// ESP32's unique setup access-point password through the web interface.
String diagStartReveal();
const char *diagRevealStatus(const String &token);
bool diagConsumeReveal(const String &token);
void diagCancelReveal(const String &token);
bool diagInterceptTouch(bool touched, unsigned long now);

