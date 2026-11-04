# Touch Light V1.2.8 — Engineering and Web Interface Enhancements
Based on V1.2.7. Firmware changes only; hardware Rev 1H unchanged.

1. Engineering page board ID, hardware revision, firmware type/version, controller, touch and PWM GPIO, sourced from firmware constants.
2. Raised 20-pixel Wi-Fi signal LED with existing strength colors.
3. Brighter active ON (red) and OFF (green) buttons.
4. Metallic blue raised brightness fader with a mobile-friendly slider target.
5. Compact metallic ENGINEERING navigation button.
6. Saved phone/browser name confirmation and persistent current-name display.
7. Optional uniform 880 Hz, 75 ms web-button beep, browser-local preference in Engineering; enabled by default.

**Testing status:** Not compiled or tested on physical ESP32 hardware yet. Verify Arduino compilation, AP/STA, Home Assistant MQTT, touch, PWM, all web controls, and diagnostics before deployment. The physical touch pad has no sound. Browser sound depends on device audio settings and browser gesture rules.
