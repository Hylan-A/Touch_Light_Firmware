# TECH 49 LAB — Touch Light 0008

Universal firmware **V1.2.5** for the ESP32-C3 touch light. Based on V1.2.3, compiled/uploaded and watchdog display confirmed working by Hylan on November 1, 2026. V1.2.4 address display was confirmed on the user's PC. V1.2.5 switches control and Engineering status polling to one second; syntax and interval checks passed, ESP32 compilation and hardware validation pending.

## Arduino setup

Open `Touch_Light_0008/Touch_Light_0008.ino`. Select **ESP32C3 Dev Module**, enable **USB CDC On Boot**, and select the board's COM port. Keep the ESP32 core and library settings used for the working installation.

The control page displays each unit's home web address, for example `http://tech49-light-b2f180.local`. Share it with another phone on the same home network. The IP address remains available as a fallback.

## Features

- Touch on/off and brightness ramp; web control and optional Home Assistant/MQTT.
- Browser names in diagnostic command entries.
- One-second status refresh on control and Engineering pages.
- Console chase strip, live diagnostics, color/text report exports.
- 60-second loop watchdog, lifetime and resettable test counters.
- Controller restart status indicator.
- Engineering watchdog dot: half-second green, half-second dark, enabled by fresh reported watchdog feed progress.

Wi-Fi and broker credentials are entered during setup and stored on the controller. They are not embedded in the source.

The sketch folder contains source and previous change notes. iPhone printing and automatic recovery under forced faults require separate validation; the feed indicator alone does not prove watchdog reset operation.
