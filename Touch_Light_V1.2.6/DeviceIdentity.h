#pragma once
#include <stdint.h>
#include <stdio.h>

// Arduino ESP.getEfuseMac() stores MAC octet 0 in the least-significant byte.
// Format all six octets in network order; never truncate to the vendor prefix.
// Output is 12 hex characters plus NUL, stable across reboot/settings resets.
inline void touchLightDeviceId(uint64_t efuseMac, char (&out)[13]) {
  snprintf(out, sizeof(out), "%02X%02X%02X%02X%02X%02X",
           static_cast<unsigned>((efuseMac >> 0) & 0xFF),
           static_cast<unsigned>((efuseMac >> 8) & 0xFF),
           static_cast<unsigned>((efuseMac >> 16) & 0xFF),
           static_cast<unsigned>((efuseMac >> 24) & 0xFF),
           static_cast<unsigned>((efuseMac >> 32) & 0xFF),
           static_cast<unsigned>((efuseMac >> 40) & 0xFF));
}
