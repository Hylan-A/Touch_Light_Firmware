# Touch Light V1.2.9 — Engineering UI update (test build)

Based on V1.2.8. Changes: five-bar RSSI indicator; metallic-blue sound toggle and ON/OFF status; two-second continuous hold to reset test counts or restart; remove More Settings on main page; browser naming and save confirmation relocated to Engineering > Browser preferences.

## Important hardware configuration check
The uploaded V1.2.8 BoardConfig.h has TOUCH_PIN=43, LED_PWM_PIN=3 and TOUCH_INPUT_MODE=INPUT_PULLDOWN, despite comments referring to ESP32-C3. These are **not the production C3 pin assignments**. This version preserves the uploaded configuration rather than silently changing hardware behavior. **Before uploading to a production XIAO ESP32-C3, check BoardConfig.h against the actual board** (earlier confirmed C3 touch GPIO21, PWM GPIO4, touch INPUT_PULLUP with no external resistor). Do not flash this as-is to a C3. Board ID is also still "0007 LAB RAT" and must be set appropriately.

## Testing
Uncompiled source; requires Arduino IDE compilation and device/browser testing. Verify long-press cancellation, button sound, browser label, Wi-Fi bars and normal lamp functions.
