# Touch Light V1.2.9 — Engineering UI update (test build)

Based on V1.2.8. Changes: five-bar RSSI indicator; metallic-blue sound toggle and ON/OFF status; two-second continuous hold to reset test counts or restart; remove More Settings on main page; browser naming and save confirmation relocated to Engineering > Browser preferences.

## Important hardware configuration check
The uploaded V1.2.8 BoardConfig.h has TOUCH_PIN=43, LED_PWM_PIN=3 and TOUCH_INPUT_MODE=INPUT_PULLDOWN, despite comments referring to ESP32-C3. These are **not the production C3 pin assignments**. This version preserves the uploaded configuration rather than silently changing hardware behavior. **Before uploading to a production XIAO ESP32-C3, check BoardConfig.h against the actual board** (earlier confirmed C3 touch GPIO21, PWM GPIO4, touch INPUT_PULLUP with no external resistor). Do not flash this as-is to a C3. Board ID is also still "0007 LAB RAT" and must be set appropriately.

## Testing
Uncompiled source; requires Arduino IDE compilation and device/browser testing. Verify long-press cancellation, button sound, browser label, Wi-Fi bars and normal lamp functions.

## Engineering/front-page refinement (November 4, 2026)
- Home Wi-Fi and MQTT status tiles on the lamp page now open the existing `/setup` configuration form.
- Lamp page shows the same live five-bar RSSI thresholds and colors as Engineering, using an added `rssi` field in `/api/state`.
- Engineering has Configure Wi-Fi and Configure MQTT links alongside their status rows.
- Removed redundant Browser Preferences Wi-Fi link and the large green LIVE status bar. Pause/Resume still works.
- MQTT shortcut expands the existing optional MQTT fields; no credentials or settings are changed until Save & connect is submitted.
- Engineering signal indicator unchanged.
