#pragma once
#include <Arduino.h>

// Sets up per-unit Wi-Fi provisioning and iPhone-compatible web controls.
void webControlBegin();
void webControlLoop();

// Callbacks implemented in Touch_Light_0008.ino.
void webControlGetState(bool &powerOn, uint8_t &brightness);
void webControlCommand(bool powerValid, bool powerOn, bool brightnessValid, uint8_t brightness, const char *clientName = nullptr);

// Separate callback prevents Home Assistant commands from being mislabeled WEB.
void mqttControlCommand(bool powerValid, bool powerOn, bool brightnessValid, uint8_t brightness);

