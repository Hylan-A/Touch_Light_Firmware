#include "Diagnostics.h"
#include <esp_system.h>
#include <stdarg.h>

namespace {
constexpr uint8_t LOG_CAPACITY = 36;
constexpr size_t LOG_LENGTH = 128;
char lines[LOG_CAPACITY][LOG_LENGTH] = {};
unsigned long stamps[LOG_CAPACITY] = {};
uint8_t head = 0;
uint8_t count = 0;

// The challenge is initiated in the browser, then confirmed by deliberately
// holding the physical capacitive pad. A random token protects against a
// different browser on the LAN claiming the confirmation.
String currentToken;
bool revealPending = false;
bool revealConfirmed = false;
unsigned long pendingAt = 0;
unsigned long confirmedAt = 0;
unsigned long touchStartedAt = 0;
bool consumeTouchUntilRelease = false;
constexpr unsigned long CONFIRM_TOUCH_MS = 4000;
constexpr unsigned long REQUEST_WINDOW_MS = 45000;
constexpr unsigned long SECRET_WINDOW_MS = 25000;

String jsonEscape(const char *text)
{
  String result;
  for(const unsigned char *p=reinterpret_cast<const unsigned char*>(text);*p;p++){
    if(*p=='"' || *p=='\\') {
      result+='\\'; result+=static_cast<char>(*p);
    } else if(*p<0x20){
      char escape[7];
      snprintf(escape,sizeof(escape),"\\u%04x",static_cast<unsigned>(*p));
      result+=escape;
    } else result+=static_cast<char>(*p);
  }
  return result;
}

void expireRevealIfNecessary(unsigned long now)
{
  if(revealPending && now-pendingAt>REQUEST_WINDOW_MS) {
    revealPending=false;
    revealConfirmed=false;
    currentToken="";
    touchStartedAt=0;
  } else if(revealConfirmed && now-confirmedAt>SECRET_WINDOW_MS){
    revealPending=false;
    revealConfirmed=false;
    currentToken="";
  }
}
}

void diagLog(const char *message)
{
  if(message==nullptr)return;
  snprintf(lines[head],LOG_LENGTH,"%s",message);
  stamps[head]=millis();
  head=(head+1)%LOG_CAPACITY;
  if(count<LOG_CAPACITY)count++;
}

void diagLogf(const char *format,...)
{
  char line[LOG_LENGTH];
  va_list args;
  va_start(args,format);
  vsnprintf(line,sizeof(line),format,args);
  va_end(args);
  diagLog(line);
}

void diagClearEvents()
{
  // Reset ring-buffer index and count; the next event starts a clean list.
  // This does not affect Wi-Fi, MQTT, NVS, or physical touch operation.
  head=0;
  count=0;
}

String diagEventsJson()
{
  String json="[";
  json.reserve(3400);
  const uint8_t beginning=(head+LOG_CAPACITY-count)%LOG_CAPACITY;
  for(uint8_t i=0;i<count;i++){
    const uint8_t n=(beginning+i)%LOG_CAPACITY;
    if(i)json+=",";
    json+="{\"ms\":";
    json+=String(stamps[n]);
    json+=",\"msg\":\"";
    json+=jsonEscape(lines[n]);
    json+="\"}";
  }
  json+="]";
  return json;
}

String diagStartReveal()
{
  // 128-bit unpredictable challenge. Replaces a previous unclaimed challenge.
  char token[33];
  snprintf(token,sizeof(token),"%08lX%08lX%08lX%08lX",
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()));
  currentToken=token;
  revealPending=true;
  revealConfirmed=false;
  pendingAt=millis();
  touchStartedAt=0;
  diagLog("[SECURITY] Setup-code reveal requested; hold touch for 4 seconds");
  return currentToken;
}

const char *diagRevealStatus(const String &token)
{
  expireRevealIfNecessary(millis());
  if(token.isEmpty() || token!=currentToken)return "invalid";
  if(revealConfirmed)return "confirmed";
  return revealPending ? "waiting" : "invalid";
}

bool diagConsumeReveal(const String &token)
{
  if(strcmp(diagRevealStatus(token),"confirmed")!=0)return false;
  revealPending=false;
  revealConfirmed=false;
  currentToken="";
  diagLog("[SECURITY] One-time setup Wi-Fi password revealed after physical approval");
  return true;
}

void diagCancelReveal(const String &token)
{
  if(!token.isEmpty() && token==currentToken) {
    revealPending=false;
    revealConfirmed=false;
    currentToken="";
    touchStartedAt=0;
    diagLog("[SECURITY] Setup-code reveal canceled");
  }
}

bool diagInterceptTouch(bool touched,unsigned long now)
{
  expireRevealIfNecessary(now);
  // After any confirmation, consume the release of the confirming touch
  // so it cannot also turn a light ON/OFF or adjust brightness.
  if(consumeTouchUntilRelease){
    if(!touched)consumeTouchUntilRelease=false;
    return true;
  }
  if(!revealPending || revealConfirmed)return false;
  if(!touched){
    touchStartedAt=0;
    return true;
  }
  if(touchStartedAt==0)touchStartedAt=now;
  if(now-touchStartedAt>=CONFIRM_TOUCH_MS){
    revealConfirmed=true;
    revealPending=false;
    confirmedAt=now;
    consumeTouchUntilRelease=true;
    diagLog("[SECURITY] Physical touch confirmation accepted");
  }
  return true;
}

