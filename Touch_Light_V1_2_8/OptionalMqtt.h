#pragma once
#include <Arduino.h>

// Optional Home Assistant integration over the *same* household Wi-Fi.
// Enabled only if the homeowner opts in during setup.
void optionalMqttBegin(const String &unitSuffix);
void optionalMqttLoop();
void optionalMqttPublishState(bool on, uint8_t brightness);
bool optionalMqttSaveSettings(bool enabled,
                              const String &brokerHost, uint16_t brokerPort,
                              const String &username, const String &password);
void optionalMqttEraseSettings();
bool optionalMqttEnabled();
bool optionalMqttConnected();

