# Tech 49 Lab Touch Light 0008 V1.2.5 (test)

Adds a visible, clickable Web address beneath the light name on the control page. The firmware builds this address from the actual unit hostname; B2F180 shows http://tech49-light-b2f180.local. Share it with phones on the same home network. The existing mode and IP display remains available as a fallback if local hostname resolution is unavailable. On a direct connection, use 192.168.4.1; the displayed home address is for use after home Wi-Fi setup.

Includes the V1.2.3 half-second watchdog blink confirmed working by Hylan.

Extract and open Touch_Light_0008.ino with the same Arduino board/core settings used for V1.2.3. Compile/upload and reopen the control page. V1.2.5 JavaScript syntax and mocked address display checks passed; ESP32 compilation and hardware testing pending.

V1.2.5: Both light-control status and Engineering diagnostics refresh every 1000 ms. Watchdog display remains 500 ms on / 500 ms off. JavaScript syntax and refresh interval checks passed; ESP32 compilation and multi-browser hardware validation pending.
