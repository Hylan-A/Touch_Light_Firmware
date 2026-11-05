# Tech 49 Lab Touch Light V1.3.0 — Connection UI test build

Based on V1.2.9 Engineering Refined.

- Main page Wi-Fi and MQTT boxes are status-only. Wi-Fi signal bars and RSSI remain. MQTT shows saved broker hostname/IP.
- Engineering page retains separate Configure Wi-Fi and Configure MQTT buttons.
- MQTT has a dedicated `/mqtt` configuration screen, accessible over home Wi-Fi or Direct Wi-Fi, with saved non-secret values prefilled.
- Wi-Fi `/setup` can be opened on either network and retains MQTT settings when saving Wi-Fi.
- Saving either configuration requires 4-second physical touch confirmation using the existing one-time challenge system.
- Saving MQTT restarts controller to apply new broker settings without erasing Wi-Fi credentials.
- Saved MQTT password is not shown; leaving the field empty retains it.
- Existing touch and PWM implementation was not intentionally changed.

## Test sequence

1. Compile for LAB RAT 0007 ESP32-S3 (check BoardConfig.h) and flash over USB.
2. Check front page Wi-Fi/MQTT boxes are status-only; test touch and brightness.
3. From home Wi-Fi, open Engineering > Configure MQTT, edit broker and save; hold touch pad 4 seconds when prompted. Confirm restart and MQTT status.
4. From home Wi-Fi, open Engineering > Configure Wi-Fi, verify current SSID is prefilled; do not save unless changing settings.
5. Test both setup screens from Direct Wi-Fi, if needed. Verify no redirect/403 on MQTT.

**Status:** Source prepared; JavaScript syntax checks performed. Arduino compilation and physical board testing NOT performed. Do not deploy to production until verified on LAB RAT.
