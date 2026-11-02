# Tech 49 Lab Touch Light — V1.2.6 (test)

Removes the fixed board-0008 startup banner; startup now says TECH 49 LAB - Touch Light. The sketch folder retains its project name.

Fixes duplicate device identities. V1.2.5 masked the low 24 bits of ESP.getEfuseMac(), selecting the manufacturer prefix in reverse byte order. V1.2.6 uses all six factory MAC bytes, formatted in normal byte order as 12 uppercase hex digits. The full MAC avoids truncation collisions across different manufacturer prefixes. Each normally provisioned ESP32 receives a stable hardware-derived ID.

The ID supplies the device name, setup SSID, lowercase mDNS hostname, MQTT client ID/topics and Home Assistant discovery identity. Example only: MAC 80:F1:B2:12:34:56 produces Tech49-Light-80F1B2123456 and http://tech49-light-80f1b2123456.local. Read YOUR actual ID from Serial Monitor after upload.

## Install and migrate

Open Touch_Light_0008/Touch_Light_0008.ino with the same ESP32-C3 settings used for V1.2.5. Compile/upload over USB. No intentional erase of home Wi-Fi, MQTT settings, setup password, reset counters or test baseline occurs. Hold-to-reset behavior is unchanged.

Board 0012: upload this fix before initial Wi-Fi setup. Record the new setup SSID and existing per-board setup password from Serial Monitor at 115200 baud. Join that network and open http://192.168.4.1 to configure the owner's home Wi-Fi.

Board 0008: saved home Wi-Fi and MQTT credentials remain; hostname and MQTT identity change. Open its new hostname (or router-assigned IP), update Home Screen/browser shortcuts, and name the browser again for the new address. Home Assistant discovers a new entity when MQTT is enabled. Review entity IDs in dashboards/automations. After verifying the new entity, remove the obsolete B2F180 entity/discovery entry using Home Assistant or MQTT management; this firmware does not automatically delete old retained discovery records, because another old board may still use them.

Keep using one-second web refresh and half-second watchdog blink.

## Validation

Actual DeviceIdentity.h tested with a host C++ compiler: same vendor/different serial, different vendor/same serial, leading zeroes, expected byte order and repeated-call stability. Both embedded page scripts pass JavaScript syntax checks. ESP32 compilation and on-board validation pending. Verify boards 0008 and 0012 now print different IDs before operating both together.
