# TECH 49 LAB — Touch Light 0008

Universal firmware **V1.2.3** for the ESP32-C3 touch light. Compiled, uploaded, and watchdog display confirmed working by Hylan on November 1, 2026.

## Arduino setup

Open `Touch_Light_0008/Touch_Light_0008.ino`. Select **ESP32C3 Dev Module**, enable **USB CDC On Boot**, and select the board's COM port. Keep the ESP32 core and library settings used for the working installation.

## Features

- Touch on/off and brightness ramp; web control and optional Home Assistant/MQTT.
- Browser names in diagnostic command entries.
- Console chase strip, live diagnostics, color/text report exports.
- 60-second loop watchdog, lifetime and resettable test counters.
- Controller restart status indicator.
- Engineering watchdog dot: half-second green, half-second dark, enabled by fresh reported watchdog feed progress.

Wi-Fi and broker credentials are entered during setup and stored on the controller. They are not embedded in the source.

The sketch folder contains source and previous change notes. iPhone printing and automatic recovery under forced faults require separate validation; the feed indicator alone does not prove watchdog reset operation.
