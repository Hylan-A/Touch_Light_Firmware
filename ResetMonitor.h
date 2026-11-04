#pragma once
#include <Arduino.h>

// Tech 49 Lab V1.1.5 reset diagnostics and automatic loop-task recovery.
// Historical counts and the one-time web-restart marker are kept in NVS;
// the 36-entry live event log remains RAM-only.
void resetMonitorBegin();
void resetMonitorArmWatchdog();
void resetMonitorFeedWatchdog();
bool resetMonitorMarkWebRestart();

const char *resetMonitorLastCause();
uint32_t resetMonitorBootCount();
uint32_t resetMonitorWatchdogCount();
uint32_t resetMonitorWebRestartCount();
bool resetMonitorWatchdogArmed();


uint32_t resetMonitorFeedCount();
String resetMonitorTestCountsJson();
bool resetMonitorResetTestCounts(const String &epoch);
