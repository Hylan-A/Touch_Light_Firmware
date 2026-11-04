# Tech 49 Lab — Touch Light 0008 — Universal V1.1.5 Automatic Recovery (TEST BUILD)

**STATUS:** This V1.1.5 firmware has been written to a **new branch**
`touchlight-v115-watchdog-reset-diag` and its embedded browser JavaScript
passes a syntax check. **It has NOT yet been compiled in Arduino IDE,
uploaded, or watchdog-tested on the physical XIAO ESP32-C3.**
The owner's existing **V1.1.4 branch remains unchanged** (including the
final `right:-24px;top:-24px` Exit button placement). V1.1.4 is the
bench-tested rollback version, including successful iPhone text/email
delivery of a saved diagnostic log.

## V1.1.5 — Watchdog, reset cause and remote Restart

- **60-second Task Watchdog:** `ResetMonitor.cpp` configures the
  ESP-IDF 5.x Task Watchdog to monitor the Arduino loop task after normal
  `setup()` completes. The watchdog is fed only by normal loop progress,
  including the setup-password reveal touch-handling path. A stalled
  loop can trigger a panic and automatic reboot. The ESP32-C3 CPU0 idle
  task remains monitored as well. Recovery is not guaranteed for all
  hardware failures, severe supply issues or failures of the watchdog
  subsystem itself.
- **Boot/reset reason:** `esp_reset_reason()` distinguishes power cycle,
  reset-pin action, software crash/panic, task/interrupt watchdog, brownout
  and intentional web software restart (via one-time NVS marker).
  Each boot adds a fresh `[RESET]` message to the regular 36-event RAM
  log, so the previous reset is still identifiable after RAM clears.
- **Persistent counters:** Under the separate `t49reset` Preferences
  namespace, the firmware records total boots, watchdog resets and
  web-triggered restarts. Values are updated only at boot; the web restart
  pending marker is written only when explicitly requested. Existing
  home Wi-Fi, MQTT settings and per-device setup passwords are untouched.
  Routine events are NEVER written continuously to flash.
- **Diagnostics UI:** A controller recovery card displays watchdog active
  status, previous reset cause, approximate last boot time, and persistent
  counts. The last boot time uses the **iPhone's own clock minus the board's
  uptime**, without a router or internet. It is *not* a permanent record
  of historical wall-clock times. A fresh boot after a watchdog reset
  appears in the recent RAM log, time-stamped when the phone views it.
- **Restart Controller:** Amber button near the bottom of Diagnostics.
  A confirmation explains that the light will restart OFF. A POST to
  `/api/diag/restart` saves a one-time web-restart marker and schedules
  `ESP.restart()` 2 seconds after acknowledging the request. The
  resulting reboot is logged and counted as a **Web-requested software
  restart** at next boot. This is NOT a factory reset and cannot recover
  a completely unresponsive web server.
- **Saved diagnostic export** now includes prior reset reason, counters,
  watchdog status and an approximate boot time, alongside the latest
  36 RAM log entries. It contains no stored Wi-Fi/MQTT passwords.
- **No authentication added:** The remote restart endpoint is accessible
  to devices able to reach the ESP32 web server on the local Wi-Fi or
  password-protected direct AP. Do **not** expose this controller directly
  to the public internet. An admin PIN could be considered for later
  deployments.
- **Preserved:** Physical touch and dimming, PWM GPIO4, touch GPIO21,
  local iPhone control, optional HA/MQTT, direct AP provisioning, password
  reveal with physical hold, PWM percentage logs, red Pause/green Resume,
  Clear Log, Save Log, and Exit button `right:-24px;top:-24px`.

### Bench test before treating V1.1.5 as final

1. Download this branch and open `Touch_Light_0008.ino` from its
   `Touch_Light_0008` directory. Arduino IDE 2.3.x, ESP32 board package
   3.x, board **ESP32C3 Dev Module**, USB CDC on boot, PubSubClient as
   in the successful V1.1.4 flashing.
2. **Verify** in Arduino IDE first. If the build fails, do NOT overwrite
   the known-working board firmware. Retain V1.1.4 for rollback.
3. Upload to TL-0008 without erasing flash, then open Serial Monitor at
   115200 baud. Confirm the `[RESET]` last-cause message and
   `[WATCHDOG] Arduino loop task monitored; 60-second reset timeout.`
   A watchdog initialization error is **not** a successful test.
4. Check original touch/dim, iPhone lamp controls, Home Assistant MQTT,
   direct setup-mode behavior (if practical), diagnostics and Save Log.
5. On Diagnostics verify **Watchdog: Active (60 seconds)**; note the
   initial recorded counts. Press **Restart Controller**, accept
   confirmation, wait for the lamp to disconnect/reconnect. It should
   restart **OFF**, while stored Wi-Fi and MQTT reconnect normally.
6. Reopen Diagnostics: last reset cause should say
   **Web-requested software restart**, the **Web restarts** counter
   should increment by one, and the 36-event RAM log should include
   `[RESET] Previous reset: Web-requested software restart`. Save and
   share the text log to ensure reset details export correctly.
7. Press the board's physical RESET (if safe to access) to confirm
   it is *not* misclassified as a web restart.
8. **Watchdog timeout recovery remains unverified** until tested under
   a deliberately stalled loop on a spare/bench board with a safe
   one-shot test harness. Do not deliberately freeze the shipped
   firmware or leave a test-induced boot loop in production.
9. If Wi-Fi reconnection, touch, or watchdog behaviour is unstable,
   flash your saved **V1.1.4** version and investigate before release.

## V1.1.4 — Log controls and lamp Exit help

- **PAUSE / RESUME:** In the diagnostics log, **Pause is red** and shows a
  **red PAUSED status**. Tap to freeze the displayed log. The button then
  becomes **green Resume**, and tapping it restores the **green LIVE
  status** and refreshes the view. This only pauses display updates;
  the ESP32 continues recording diagnostic events into its 36-entry
  circular RAM buffer while paused.
- **CLEAR LOG:** Gray control with confirmation. A POST command clears the
  device's **RAM log buffer only**. Normal light operation, Wi-Fi settings,
  and Home Assistant MQTT configuration are not changed. New events start
  appearing immediately afterward.
- **SAVE LOG:** Blue control. Saves the latest available 36 events as a
  timestamped plain-text diagnostic file using the iPhone's local clock.
  No passwords are written into that file. iPhone users can look in
  Safari Downloads / Files, though exact download handling varies by
  browser and Home Screen web-app mode. It works locally without internet.
- **Navigation cleanup:** Diagnostics now has only the larger
  **← Back to Lamp** button. The redundant **✕ Close** button is removed.
  The main lamp control page now has **✕ Exit** in the upper-right corner.
  It displays instructions on how to swipe up from an iPhone Home Screen
  web app, or close a Safari tab. **It cannot actually force iOS to close
  the app**; no such browser capability exists for normal installed
  webpages.
- The physical touch controls, the HTTP control page, Home Assistant MQTT,
  RSSI quality colors, local timestamps, brightness counts and PWM percent,
  optional MQTT setup, and setup-code touch confirmation are retained.

### Suggested V1.1.4 bench verification

1. **Back up your currently working firmware**. Download this branch and
   open the `Touch_Light_0008` sketch directory in Arduino IDE.
2. **Verify** with ESP32C3 Dev Module, USB CDC On Boot enabled and
   PubSubClient installed. Upload only after a successful compilation.
   Don't erase flash: the existing household Wi-Fi and MQTT settings
   should remain.
3. In iPhone Diagnostics, check **Pause (red)**, then exercise the pad
   and Home Assistant. The displayed log should remain frozen while
   **PAUSED (red)**. Tap **Resume (green)**; the current log should appear
   with a **LIVE (green)** indicator.
4. Tap **Save Log** and confirm a text file arrives in Downloads / Files
   and includes the current device ID and recent event timestamps.
5. Tap **Clear Log**, accept confirmation, and verify the display is empty
   until another event occurs. Verify touch, Wi-Fi and MQTT remain working.
6. Confirm **Back to Lamp** works, the redundant Close button is gone,
   and the lamp's **✕ Exit** shows swipe-up instructions.
7. If web pages appear stale, close/reopen the iPhone shortcut; no Wi-Fi
   or MQTT factory reset is required.

## V1.1.3 — Final interface refinements

- Main lamp page: **System Diagnostics & Live Log** is now a large blue
  button using a normal one-tap click handler and mobile touch-action CSS.
- Diagnostics page: larger **Back to Lamp** and **✕ Close** buttons.
  Both return to the lamp controls with a single tap. The Close button
  does **not** close an iPhone Home Screen web app or Safari; iOS normally
  requires swiping home/closing the browser for that.
- Live diagnostic events now show **8-bit PWM counts and an approximate
  duty-cycle percentage**. Examples:
  `[WEB] Light ON, brightness 110/255 (43% PWM)`,
  `[TOUCH] Dimming stopped, brightness 200/255 (78% PWM)`, and
  `[HOME ASSISTANT] Light OFF, saved 63/255 (25%); output 0%`.
- The displayed percentage is calculated as
  `round(count * 100 / 255)`. This is *PWM duty cycle*, not measured
  LED output or human-perceived brightness.
- Duplicate no-op Home Assistant commands remain filtered as in V1.1.2.
- No changes to Wi-Fi credentials, MQTT broker settings, GPIO mapping,
  or touch/Wi-Fi provisioning are intended.

### V1.1.3 bench test

1. **Back up working V1.1.2**, retaining the exact Arduino
   `Touch_Light_0008` sketch folder name. Download this branch and extract
   its `Touch_Light_0008` directory into a separate folder.
2. Open `Touch_Light_0008.ino` in Arduino IDE; select **ESP32C3 Dev
   Module**, **USB CDC On Boot = Enabled**, and your connected port.
3. Click **Verify**. If successful, **Upload** without a flash erase.
   Existing household Wi-Fi and optional MQTT settings should remain.
4. On iPhone, open main lamp controls. Tap the blue diagnostics button
   **once** and verify immediate navigation. In Diagnostics tap
   **✕ Close** and **Back to Lamp** and verify each returns on one tap.
   If a cached page appears, close/reopen the iPhone Home Screen shortcut.
5. Switch the light ON/OFF using the pad, iPhone, and Home Assistant.
   Adjust brightness with all three. Confirm each event contains the
   correct source tag, PWM count/255, and approximate PWM percentage.
   OFF events should say the preset and **output 0%**.
6. Confirm **Wi-Fi quality color indicator**, RSSI, local timestamps,
   and Home Assistant connection still work.
7. Test setup-code reveal only if desired; don't share its unique password
   in public screenshots.

If navigation still needs two taps on this firmware, test directly in
Safari and report iOS/browser behavior so it can be diagnosed rather than
treated as a deliberate double-tap feature.

## V1.1.2 — Avoid duplicate Home Assistant dimming log entries

**This is a new test revision.** Board 0008's V1.1.1 was observed to
occasionally record **two identical [HOME ASSISTANT] entries** for each
brightness-slider adjustment, even though the light works normally.

Home Assistant's MQTT light integration may send multiple commands for one
user gesture (e.g., a brightness value plus `ON`). The firmware previously
logged **every received remote command**, even when the requested state
was already in effect. The updated code first applies each command, then
compares the full working state (on/off, brightness and saved brightness)
to its previous values. If nothing changed, it omits that event and
does not republish an unchanged MQTT state.

Actual brightness changes, ON/OFF transitions, and changes to the stored
brightness remain logged and continue synchronizing with Home Assistant.
The same no-op filter also applies to browser controls.

**Bench test before distributing:** Adjust brightness repeatedly in Home
Assistant and confirm each distinct state change creates one log entry,
not two identical entries. Check physical touch, browser control and HA
remain synchronized. This fix is based on source analysis and has not
yet been compiled or tested on the physical board in this branch.

The V1.1.1 Wi-Fi quality features (green Excellent, blue Good, yellow
Marginal, red Weak and gray Offline) are all retained, as are local
12-hour timestamps and password recovery.

## V1.1.1 Wi-Fi signal quality indicator — new test branch

The iPhone **System Diagnostics & Live Log** page now shows a
color-coded signal indicator **in addition to the numeric RSSI**:

| Received RSSI | Quality label | Indicator |
| --- | --- | --- |
| −60 dBm or stronger | Excellent | Green |
| −61 to −70 dBm | Good | Blue |
| −71 to −80 dBm | Marginal | Yellow |
| Below −80 dBm | Weak | Red |
| Wi-Fi disconnected | Not connected | Gray |

This is a *rough coverage guide*, not a measure of packet loss,
network congestion, or router performance. The indicator refreshes with
the existing diagnostics update every two seconds. The user sees both
the descriptive label **and** the dot color for accessibility.
For example, board TL-0008 previously measured **−51 dBm — Excellent**.

No new ESP32 library, changes to GPIO wiring or credentials, or configuration
reset are required. Just keep a backup of the working V1.1 and compile/upload
the sketch from this branch. Stored Wi-Fi and optional MQTT configuration are
normally retained across an ordinary upload. After uploading, reload the
iPhone diagnostic page; if an old cached page appears, close and reopen it.

**This variant has been JavaScript syntax-checked and the RSSI range mapping
has been tested in isolation. It still needs Arduino compilation and an
actual board 0008 bench test.**

## V1.1 Diagnostics — new, not yet hardware tested

From the light's browser control panel, tap **System Diagnostics & Live Log**,
or open `http://tech49-light-<device-id>.local/diagnostics` on the same
household Wi-Fi. No computer or USB serial cable is needed.

The new diagnostics page displays:
- The unique ESP32-C3 device ID and firmware version.
- Current Wi-Fi connection and LAN IP address.
- Actual **Wi-Fi RSSI in dBm**, or Unavailable if disconnected.
- Direct/setup Wi-Fi availability.
- Optional MQTT enabled and connected/disconnected status.
- Recent timestamped events tagged **[TOUCH]**, **[WEB]**,
  **[HOME ASSISTANT]**, **[WIFI]**, **[MQTT]** and **[SECURITY]**.

The web log is **a dedicated event history, not a byte-for-byte mirror of
Arduino's Serial Monitor**. It stores at most 36 recent events in RAM, and
clears on reboot. The web page refreshes every two seconds.

### Local clock timestamps (new variant)

On the **iPhone diagnostics page**, each event now displays an actual
local time such as **7:23:05 PM** instead of seconds/hours since startup.
The page also shows the iPhone's local date and time, while retaining a
separate ESP32 uptime reading for engineering diagnostics.

The board still records event times using `millis()`. When the phone requests
the event history, the browser calculates the local clock time from the
device uptime and the iPhone's current clock. This works through either home
Wi-Fi or the lamp's direct Wi-Fi even **without internet, DNS or NTP**.
It honors the phone's local timezone and daylight saving time. If the
iPhone's clock is incorrect, the event times will be incorrect. Logs remain
volatile and clear on board restart; these are not permanent timestamps stored
on the ESP32.

The **USB Arduino Serial Monitor** is separate: its left-hand timestamps
already come from the attached computer's clock.

### Recover the unique setup password without a laptop

1. Open the new **Diagnostics** page while connected to the light through
   its local network (or its direct setup network).
2. Tap **Show setup information**.
3. Within 45 seconds, physically hold the lamp's touch pad for **4 seconds**.
   The confirming hold deliberately does **not** toggle/dim the lamp.
4. The iPhone displays the light's **individual** `Tech49-Light-XXXXXX`
   SSID and unique setup password, for recording on a private QR-code label.
   The password disappears after approximately 45 seconds.
5. If you do not confirm within 45 seconds, the request expires.

Security considerations: browser control has no user login, and HTTP is not
encrypted on the local network. The sensitive setup password is not included
in ordinary diagnostics responses; recovering it requires a fresh random
token **plus a physical touch hold**. Do not expose the ESP32's HTTP port to
the public internet or put private setup labels online.

### Suggested V1.1 bench tests

1. Keep the **working V1.0 RC1** sketch as a separate backup.
2. In Arduino IDE, compile the V1.1 sketch with ESP32 3.x and PubSubClient,
   then upload without erasing NVS. Note that this V1.1 revision has not been
   verified by an actual Arduino compiler yet.
3. Check that the normal iPhone page still controls the lamp.
4. Open Diagnostics on iPhone. Check actual RSSI, uptime and device ID.
5. Turn ON/OFF via physical touch and iPhone, and check the correct event tags.
6. With Home Assistant connected, send ON/OFF and brightness and verify
   **[HOME ASSISTANT]** events and all control surfaces synchronized.
7. Tap **Show setup information**. Confirm that the password stays hidden
   until a four-second physical touch hold, and that the lamp does not
   accidentally dim during that hold.
8. Disconnect/reconnect home Wi-Fi to verify disconnect reasons and
   reconnection history; touch must keep working.
9. Reboot and verify previous home Wi-Fi / optional MQTT settings are kept,
   but events from the previous boot are gone.

## One firmware, three control methods

- **Physical touch pad:** short press toggles; long press dims. Works with no
  Wi-Fi or internet.
- **iPhone or browser:** local Wi-Fi webpage with ON/OFF, brightness, live state.
- **Optional Home Assistant:** via MQTT on the *same household Wi-Fi connection*.
  Off by default. Uses unique MQTT/discovery identifiers for each ESP32-C3.
- Switching from touch/browser/HA updates the same hardware light state.
- The browser refreshes light state every 1.5 seconds. HA gets MQTT state
  publications on ON/OFF, incoming commands and at the end of touch dimming.

**No Home Assistant account or MQTT broker is required** for the first two
control methods. If HA is selected, an existing MQTT broker and working Home
Assistant MQTT integration are required.

## Hardware and programming

- Touch Light 0008 / Seeed XIAO ESP32-C3, Rev1H PCB:
  IQS228B touch sensor OUT -> D6/GPIO21; LED PWM -> D2/GPIO4.
- External **12 V LED supply** required for the LED array.
- Arduino IDE **ESP32C3 Dev Module**, ESP32 board core 3.x,
  **Tools > USB CDC On Boot > Enabled**, Serial Monitor **115200 baud**.
- Arduino Library Manager: install **PubSubClient by Nick O'Leary**.
  It is needed to compile this universal firmware even if MQTT is disabled.
- Download/extract the whole `Touch_Light_0008` directory, and open
  `Touch_Light_0008.ino`.
- Hit **Verify** before Upload. Do not erase all flash unless intended;
  a normal firmware upload preserves stored settings in ESP32 NVS.
- Keep the earlier tested firmware folder and GitHub branch backed up.

## Setup from an iPhone: first time, or after ownership reset

1. Power the lamp. If no home network is saved, it broadcasts a Wi-Fi name
   such as **Tech49-Light-B2F180**. Each unit has a *different* suffix.
2. Locate the **individual setup password** printed on that unit's sticker.
   The firmware generates a random password (e.g. `T49-XXXXXXXX`) and
   stores it in nonvolatile memory. During bench preparation the actual
   password is printed to Serial Monitor when the setup network starts.
   The old shared `Tech49Setup!` password is **not used** in this version.
3. On the iPhone open **Settings > Wi-Fi**, join that light's Wi-Fi, enter its
   labeled password. "No Internet Connection" is expected.
4. Open Safari or DuckDuckGo and type **http://192.168.4.1**.
   Automatic iPhone captive-portal popup is **not implemented/guaranteed**
   by this revision; open the URL manually.
5. To control the light without a home router, use the page now.
   Otherwise tap **Set up home Wi-Fi**:
   - Tap **Refresh network list** or wait for the scan.
   - Choose the correct **2.4 GHz** network from the list; enter its password.
     (Hidden networks can be typed manually; SSIDs are case-sensitive.)
   - Under **Optional smart-home connection**, leave the **Use Home Assistant
     (MQTT)** box **unchecked** for normal iPhone/touch use.
   - If the owner uses Home Assistant **and has a MQTT broker**, check it.
     Supply the broker **host/IP**, port (normally **1883**), and any MQTT
     username/password. The MQTT broker IP may differ from the HA server IP.
   - Tap **Save & connect**. The device stores Wi-Fi/optional MQTT settings
     and restarts.
6. Reconnect the iPhone to the same home Wi-Fi. Browse to the name printed
   in Serial Monitor, e.g. **http://tech49-light-b2f180.local**, or to its
   actual LAN IP. The old direct-mode URL is no longer the everyday address.
7. In Safari: **Share > Add to Home Screen** to get a lamp icon.

## Home Assistant

- The ESP32-C3 joins the *same* home Wi-Fi used by the web browser.
- MQTT is **opt-in** and is not contacted when disabled.
- Per-device MQTT topics are based on the ESP32 hardware suffix:
  `tech49/light/<suffix>/cmd/power`,
  `tech49/light/<suffix>/cmd/brightness`,
  `tech49/light/<suffix>/state/power`,
  `tech49/light/<suffix>/state/brightness`, and
  `tech49/light/<suffix>/status`.
- Discovery: `homeassistant/light/tech49_light_<suffix>/config`.
- A light appears under the HA MQTT integration when the HA broker/discovery
  are configured and the ESP32 connects successfully.
- You do **not** need two different Wi-Fi connections, two ESP32s or two
  firmware images. Web and MQTT share the household Wi-Fi.
- Touch controls keep working even if the Wi-Fi router or MQTT broker fails.
- This version does not automatically import credentials from the older
  Home Assistant firmware; enter broker settings in the iPhone setup form.
- The old HA discovery ID `touch_light_0008` may remain as a stale HA
  device when migrating. Remove only after verifying the new entity works.

## Factory reset before gifting or transferring a unit

Hold the physical touch pad while **restarting/powering the ESP32-C3**, and
continue holding for approximately **8 seconds**. The firmware clears stored
household Wi-Fi, MQTT broker address, and MQTT credentials, and returns to
setup access-point mode. **The unique setup AP password stays the same**, so
the label remains valid. Check Serial Monitor for successful reset.

If the home router is unavailable, the lamp retries then broadcasts its
setup/direct Wi-Fi again. The physical touch pad never depends on Wi-Fi.

## Security and limitations

- This is prototype firmware. **Complete bench tests before gifting units**:
  on/off, hold dimming, browser slider, Wi-Fi setup scan, HA off/on, recovery
  from router/broker outage, factory reset, and full power-cycle.
- HTTP browser controls are **not authenticated**. Anyone on the trusted home
  Wi-Fi can operate the light. Do not expose/port-forward it to the internet
  or connect it to untrusted networks.
- Initial setup runs over a unique-password protected WPA2 access point, but
  HTTP/MQTT (port 1883) are not end-to-end encrypted. Configure a trusted
  LAN only; strong Wi-Fi and MQTT passwords recommended.
- Each recipient should receive a sticker with the unique SSID, unique setup
  password, direct setup URL, and a simple instruction card. Do not share an
  identical setup password for a whole production batch.
- Before distributing, validate power supply, LED current/temperature,
  mains isolation where applicable, electrical enclosure, and user safety.
- Remote control over the internet is intentionally not included.
- The optional MQTT client uses PubSubClient's synchronous connection API;
  an unreachable broker can briefly delay the microcontroller main loop
  during a retry. Bench-test failure/recovery behavior.

## Rollout and version history

- **Working backup**: `touch-light-0008-iphone-web` — direct/home Wi-Fi tested.
- **Wi-Fi setup prototype**: `touch-light-0008-friendly-wifi-setup` — network scan.
- **Functionally tested V1.0 RC1**: `touch-light-0008-universal` — scanner +
  optional Home Assistant + unique per-unit setup password.
- **V1.1 Diagnostics**: `touch-light-0008-v1.1-diagnostics` — iPhone
  diagnostics, event log, RSSI and authorized setup password retrieval.
- **V1.1 Local Clock**:
  `touch-light-0008-v1.1-local-clock` — AM/PM event timestamps derived
  from the iPhone's clock; confirmed working on TL-0008.
- **V1.1.1 Wi-Fi Quality**:
  `touch-light-0008-v1.1-wifi-quality` — adds green/blue/yellow/red/gray RSSI
  status indicators while keeping the numeric dBm reading.
- **V1.1.2 MQTT Event De-duplication (functionally tested)**:
  `touch-light-0008-v1.1.2-dedup` — suppresses duplicate no-op MQTT/WEB
  command log entries without suppressing real state changes.
- **V1.1.3 Interface Polish**:
  `touchlight-ui-final-v113` — larger single-tap buttons and
  diagnostic log PWM counts plus percentages.
- **V1.1.4 Log Controls (this test branch)**:
  `touchlight-v114-log-controls` — red Pause, green Resume, Clear Log,
  Save Log and lamp-page Exit help; removes duplicate Close from Diagnostics.
- The older MQTT-specific board package is on `touch-light-0008-package`.
- All branches live in private GitHub repo `Hylan-A/Touch_Light_0010`
  under folder `Touch_Light_0008`; the default `main` branch was not changed.

