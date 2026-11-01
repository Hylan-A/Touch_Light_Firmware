# Tech 49 Lab — Touch Light 0008

## Universal V1.2.1 Console — test build

Seeed XIAO ESP32-C3, Rev1H board 0008. Touch GPIO21 and PWM GPIO4 retained.

### Changes from V1.2.0

- Blue/gray chase strip on control, Engineering diagnostics and setup pages. Independent of log pause. Respects browser Reduced Motion preference.
- Color report opens in an overlay with a persistent Back to Diagnostics button. Print / Save as PDF calls the report frame's print function. iPhone Home Screen printing may still be unsupported: open diagnostics in Safari and use Share → Print, or save the color HTML report and print it from a supported browser. No automatic printer selection.
- Engineering watchdog dot pulses only when received successful-feed count advances. Gray when updates stop. This verifies reported feeding progress, not the watchdog's ability to reset a hung chip.
- Reset Test Counts zeros test starts, watchdog and web restart counts after confirmation. Stores one baseline blob and browser-supplied timestamp in NVS; survives power cycles. Lifetime recorded totals and network settings retained. A reset adds a system log entry; it does not clear the live log or restart the controller.
- Lifetime totals shown separately beneath test counts. Recorded starts include power-on boots; watchdog and web counts are subsets, not additional starts. Totals cover recorded history since reset monitoring was installed, not unknowable earlier device life.
- Restart indicator green on successful response, flashing red after restart acceptance, green again only after a higher boot count is received. After 60 seconds it reports reconnect failure while continuing to retry. Keep the page open to see completion; a new page has no pending request state.

### Validation status

Passed JavaScript syntax and mock DOM/API behavior checks, including report return panel, test and lifetime displays, reset submission, restart confirmation, and watchdog feed pulse. Passed C++ host harness using actual ResetMonitor.cpp with simulated Preferences/ESP APIs: persistent baseline across boots, lifetime retention, timestamp validation, and successful feeds only.

**Not compiled for ESP32 in this environment. Not verified on iPhone/Safari or physical hardware.** Use Arduino IDE to compile and upload, then verify with the checklist below. V1.2.0 was previously compiled and bench tested by the user; retain its ZIP for rollback.

### Install

Extract this ZIP. Open Touch_Light_0008/Touch_Light_0008.ino in Arduino IDE. Use the same ESP32-C3 board/core settings that compiled V1.2.0 (USB CDC on boot enabled). Upload, then reopen the phone page to load V1.2.1. No intentional network-setting or lifetime-counter erase is included.

### Bench checklist

1. Confirm V1.2.1, physical touch, slider, MQTT/Home Assistant and browser name work.
2. Open Engineering; check chase animation continues while log paused. Observe watchdog dot pulse with fresh updates. Disconnect phone Wi-Fi and check indicators go stale/offline rather than continuing green.
3. Note lifetime totals; reset test counts. Confirm zero counts and date/time; restart and power-cycle, confirm saved date remains and test/lifetime counts advance correctly.
4. Request restart with page open. Check red flashing and green only after a new boot; lamp returns OFF. Pause log and repeat to verify status remains live independently.
5. Open color report and return using Back to Diagnostics. Try report print in Safari and Home Screen mode; if the latter does not open a print dialog use the documented fallback. Save Color Log and Save Text still available.

Date/time uses the phone/browser clock, not a board RTC. Printed reports exclude passwords. No GitHub repository was created or modified by this package.
