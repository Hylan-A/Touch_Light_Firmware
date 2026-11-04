# Tech 49 Lab — Touch Light 0008

## Universal V1.2.0 Console — test build

For board **TL-0008**, Seeed XIAO ESP32-C3 on Rev1H hardware.
This source is based on `Hylan-A/Touch_Light_0010`, branch
`touchlight-v115-watchdog-reset-diag`, commit
`00c590a978a3c2cc6b5470c34447c3bff8c6dab9`.
It is prepared for a dedicated **Touch_Light_0008** repository.
The existing 0010 repository and branches have not been modified.

**Status:** Source and embedded JavaScript logic checked. Not compiled,
flashed, visually verified in a real browser, or tested on ESP32 hardware.
This is a test build, not a verified production release.

### Changes

- Blue and gray spacecraft-style control, diagnostics and provisioning pages.
- Large ON/OFF buttons, numeric PWM preset percentage, segmented bar and slider.
- Engineering section contains diagnostics navigation and optional browser naming.
- Each browser may save its own label, such as `Hylan’s iPhone`, and sends it
  with light commands. Events become `[WEB: Hylan’s iPhone] Light ON...`.
- Labels are optional and limited to 48 UTF-8 bytes. Apostrophes and normal
  Unicode names work; brackets and control characters are rejected.
- Names are browser-provided labels, not verified identity or authentication.
  Stored in browser local storage for the current address; a different IP,
  hostname, browser, Home Screen storage context, or cleared website data may
  require naming again. If storage is unavailable, the name lasts for that page.
- Unnamed clients still log `[WEB]`; HA/MQTT stays `[HOME ASSISTANT]`.
- Live and exported log colors: **ON red, OFF green, system/Wi-Fi/MQTT/reset/
  watchdog amber, other events lavender**. Event text remains visible.
- **Save Text** retains the original text export. **Save Color Log** exports
  a self-contained HTML report. **Print Color Log** opens a separate report
  containing a Print / Save as PDF button. Enable background graphics in print
  settings for highlights. No setup password is included in the report.
- Export event timestamps stay anchored to the fetched snapshot, including when
  the display is paused. They are estimates from the viewing device's clock.

### Existing behavior retained

Short-touch toggle; long-touch brightness ramp; 1 kHz eight-bit PWM; in-RAM
brightness preset memory; optional MQTT/HA discovery and control; direct AP and
household Wi-Fi provisioning; scanning; saved credentials; unique AP password;
physical-hold password reveal; startup hold to reset Wi-Fi/MQTT settings;
automatic network retries; loop watchdog and reset counters; web restart;
live log pause/resume and clear; Exit help; local operation without internet.

Reboot, factory reset, and clear log remain distinct. **Restart Controller**
returns the lamp OFF without deleting settings. Clear Log empties only the
36-event RAM buffer. Factory Wi-Fi/MQTT reset still uses the physical startup
hold described in the earlier instructions. No new web factory-reset button
has been added. Watchdog, MQTT, diagnostics storage, and GPIO configuration
modules are unchanged from V1.1.5.

### Build and first trial

1. Keep your working V1.1.5 sketch and existing Arduino IDE board/port settings
   as a rollback. Open `Touch_Light_0008.ino` from this folder.
2. Use the ESP32 Arduino board package **3.x** and PubSubClient. The previous
   setup used **ESP32C3 Dev Module**, USB CDC on boot, and Serial Monitor at
   **115200 baud**. Reuse the settings that compiled your installed V1.1.5.
3. Verify/compile first. Upload to the **0008 ESP32-C3** only after compilation
   succeeds. Keep flash erase disabled to retain Wi-Fi, MQTT, setup password,
   and reset history.
4. Pins remain **GPIO21 touch** and **GPIO4 LED PWM**. The XIAO C3 has no user
   heartbeat LED configured. Check wiring before reusing on another PCB.
5. Open the same local web address. Your existing iPhone shortcut usually can
   stay: close/reopen or reload to fetch the new page. If the address changes,
   edit or recreate the shortcut. There is no separate web app to uninstall.
6. Check ON/OFF and the slider, then physical short-touch and long-touch.
   Confirm HA still controls the same light and all three methods synchronize.
7. In Engineering, name two browsers differently. Operate from both and check
   the log source names. Reload and check that each browser retains its label.
8. Check red ON, green OFF, amber diagnostics, and lavender other messages.
   Save Text, Save Color Log, and Print Color Log; inspect the report and PDF.
9. Check pause/resume, Clear Log, physical password reveal, and Restart
   Controller. Verify reset counters and reconnection after a restart.
10. Repeat the overnight test and a controlled router/network interruption to
    check recovery on hardware. The previous stable overnight run did not by
    itself prove interruption recovery or forced-hang watchdog recovery.

### Validation performed here

- Embedded control, diagnostics, and setup JavaScript passes Node syntax checks.
- Mock DOM/API execution passed: separate browser names and persistence,
  label limits, ON/OFF button state, event classification, pause/resume,
  HTML escaping, report metadata and latest events, restart and clear requests.
- Full browser rendering could not run in this environment. Arduino compiler
  and ESP32 hardware validation remain for the bench.
- See `HISTORY_V1.1.5.md` for earlier instructions and development history;
  version-specific status claims there describe the older build.
