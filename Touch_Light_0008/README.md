# Tech 49 Lab Touch Light 0008 V1.2.3 (test)

Watchdog dot: 500 ms green, 500 ms dark. Uses an explicit browser timer instead of a CSS pulse. Fresh successful-feed reports enable blinking; after seven seconds without feed progress or responses the indicator becomes gray. No watchdog timeout or control changes.

Extract and open Touch_Light_0008.ino in Arduino IDE, using your working board settings. Compile and upload. Close/reopen or refresh the Engineering page; confirm firmware V1.2.3 before checking the blink.

Validation: JavaScript syntax and simulated half-second phase/stale-update checks passed. ESP32 compilation and physical phone verification remain for the user.
