#include "ResetMonitor.h"
#include "Diagnostics.h"

#include <Preferences.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <esp_err.h>

namespace {
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 60000UL;
constexpr char NVS_NAMESPACE[] = "t49reset";
constexpr char KEY_WEB_REQUEST[] = "webReq";
bool watchdogArmed = false;
uint32_t feedCount = 0;
uint32_t bootCount = 0;
uint32_t watchdogCount = 0;
uint32_t webRestartCount = 0;
const char *lastCause = "Not yet read";

bool isWatchdogReason(esp_reset_reason_t cause)
{
  return cause == ESP_RST_TASK_WDT ||
         cause == ESP_RST_INT_WDT ||
         cause == ESP_RST_WDT;
}

const char *reasonText(esp_reset_reason_t cause)
{
  switch (cause) {
    case ESP_RST_POWERON:  return "Power-on / power cycle";
    case ESP_RST_EXT:      return "External reset (RESET pin)";
    case ESP_RST_SW:       return "Software reset (not web-requested)";
    case ESP_RST_PANIC:    return "Software crash / panic";
    case ESP_RST_INT_WDT:  return "Interrupt watchdog timeout";
    case ESP_RST_TASK_WDT: return "Task watchdog timeout";
    case ESP_RST_WDT:      return "Other watchdog timeout";
    case ESP_RST_BROWNOUT: return "Brownout / low supply voltage";
    case ESP_RST_DEEPSLEEP:return "Wake from deep sleep";
    default:               return "Other / unknown reset reason";
  }
}
} // namespace

void resetMonitorBegin()
{
  // Read hardware-reported cause first; NVS marker is only an attribution
  // hint for a software reset that our own web endpoint initiated.
  const esp_reset_reason_t reason = esp_reset_reason();
  bool webRequested = false;
  bool historySaved = false;

  Preferences p;
  if (p.begin(NVS_NAMESPACE, false)) {
    webRequested = p.getBool(KEY_WEB_REQUEST, false);
    if (webRequested) p.remove(KEY_WEB_REQUEST); // consume exactly once
    bootCount = p.getUInt("boots", 0) + 1;
    watchdogCount = p.getUInt("wdt", 0);
    webRestartCount = p.getUInt("web", 0);

    if (isWatchdogReason(reason)) watchdogCount++;
    if (reason == ESP_RST_SW && webRequested) webRestartCount++;

    // Flash writes happen once per *boot*, never in the main loop.
    const bool a = p.putUInt("boots", bootCount) != 0;
    const bool b = !isWatchdogReason(reason) ||
                   p.putUInt("wdt", watchdogCount) != 0;
    const bool c = !(reason == ESP_RST_SW && webRequested) ||
                   p.putUInt("web", webRestartCount) != 0;
    historySaved = a && b && c;
    p.end();
  }

  const bool completedWebRestart = reason == ESP_RST_SW && webRequested;
  lastCause = completedWebRestart ?
              "Web-requested software restart" : reasonText(reason);

  Serial.print("[RESET] Previous reset: ");
  Serial.println(lastCause);
  diagLogf("[RESET] Previous reset: %s", lastCause);
  diagLogf("[RESET] Recorded boots %lu, watchdog %lu, web restarts %lu",
           static_cast<unsigned long>(bootCount),
           static_cast<unsigned long>(watchdogCount),
           static_cast<unsigned long>(webRestartCount));
  if (!historySaved) {
    Serial.println("[RESET] WARNING: Could not fully save reset history to NVS.");
    diagLog("[RESET] WARNING: NVS reset history not fully saved");
  }
}

void resetMonitorArmWatchdog()
{
  // Subscribe the Arduino loop task, not a timer ISR. Trigger a panic/reset
  // rather than only printing a timeout warning. 60s tolerates setup/network
  // operations that temporarily take longer than a normal loop iteration.
  esp_task_wdt_config_t config = {};
  config.timeout_ms = WATCHDOG_TIMEOUT_MS;
  config.idle_core_mask = 1U; // ESP32-C3: CPU0 idle task
  config.trigger_panic = true;

  esp_err_t result = esp_task_wdt_init(&config);
  if (result == ESP_ERR_INVALID_STATE) {
    // ESP32 Arduino 3.x may already initialize a task WDT at startup.
    result = esp_task_wdt_reconfigure(&config);
  }
  if (result == ESP_OK) {
    const esp_err_t status = esp_task_wdt_status(nullptr);
    if (status == ESP_OK) {
      watchdogArmed = true; // task already subscribed
    } else if (status == ESP_ERR_NOT_FOUND) {
      watchdogArmed = esp_task_wdt_add(nullptr) == ESP_OK;
    }
  }

  if (watchdogArmed) {
    esp_task_wdt_reset();
    Serial.println("[WATCHDOG] Arduino loop task monitored; 60-second reset timeout.");
    diagLog("[WATCHDOG] Active: 60-second loop task timeout");
  } else {
    Serial.printf("[WATCHDOG] ERROR: Task watchdog not armed (%s).\n",
                  esp_err_to_name(result));
    diagLog("[WATCHDOG] ERROR: Recovery watchdog unavailable");
  }
}

void resetMonitorFeedWatchdog()
{
  // This is only called when the Arduino loop completes meaningful work.
  // If loop stalls, the task WDT is not fed and can reset the controller.
  if (watchdogArmed && esp_task_wdt_reset() == ESP_OK) feedCount++;
}

bool resetMonitorMarkWebRestart()
{
  // Persist the *request* before the HTTP response is returned, so the
  // next boot can identify this as a deliberate web restart. If power
  // fails in the short gap before ESP.restart(), boot reason won't be
  // ESP_RST_SW and the stale marker will be ignored/consumed.
  Preferences p;
  if (!p.begin(NVS_NAMESPACE, false)) return false;
  const bool recorded = p.putBool(KEY_WEB_REQUEST, true) != 0;
  p.end();
  return recorded;
}

const char *resetMonitorLastCause() { return lastCause; }
uint32_t resetMonitorBootCount() { return bootCount; }
uint32_t resetMonitorWatchdogCount() { return watchdogCount; }
uint32_t resetMonitorWebRestartCount() { return webRestartCount; }
bool resetMonitorWatchdogArmed() { return watchdogArmed; }


uint32_t resetMonitorFeedCount() { return feedCount; }
struct TestBaseline {
  uint32_t magic, boots, watchdog, web;
  char epoch[16];
};
static constexpr uint32_t TEST_MAGIC = 0x49313231;
static TestBaseline readTestBaseline() {
  TestBaseline b = {};
  Preferences p;
  if (p.begin(NVS_NAMESPACE, true)) {
    if (p.getBytesLength("testBase") == sizeof(b)) p.getBytes("testBase", &b, sizeof(b));
    p.end();
  }
  if (b.magic != TEST_MAGIC) b = {};
  b.epoch[sizeof(b.epoch)-1] = 0;
  return b;
}
String resetMonitorTestCountsJson() {
  const TestBaseline b = readTestBaseline();
  String j = ",\"testBoots\":" + String(bootCount >= b.boots ? bootCount-b.boots : 0);
  j += ",\"testWatchdog\":" + String(watchdogCount >= b.watchdog ? watchdogCount-b.watchdog : 0);
  j += ",\"testWeb\":" + String(webRestartCount >= b.web ? webRestartCount-b.web : 0);
  j += ",\"testResetEpoch\":\"" + String(b.epoch) + "\"";
  return j;
}
bool resetMonitorResetTestCounts(const String &epoch) {
  if (epoch.length() != 13) return false;
  for (size_t i=0;i<epoch.length();i++) if (epoch[i]<'0' || epoch[i]>'9') return false;
  TestBaseline b = {};
  b.magic=TEST_MAGIC; b.boots=bootCount; b.watchdog=watchdogCount; b.web=webRestartCount;
  epoch.toCharArray(b.epoch,sizeof(b.epoch));
  Preferences p;
  if (!p.begin(NVS_NAMESPACE,false)) return false;
  const bool ok=p.putBytes("testBase",&b,sizeof(b))==sizeof(b);
  p.end();
  if (ok) diagLog("[RESET] Test counts reset; lifetime totals retained (time supplied by browser)");
  return ok;
}
