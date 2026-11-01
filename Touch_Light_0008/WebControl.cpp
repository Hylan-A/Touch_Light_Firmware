#include "WebControl.h"
#include "BoardConfig.h"
#include "OptionalMqtt.h"
#include "Diagnostics.h"
#include "DiagnosticsPage.h"
#include "ResetMonitor.h"

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_timer.h>

// Browser controls and optional MQTT share one household Wi-Fi connection.
// Unique per-device setup AP passwords are generated and stored in NVS.
// Print this password on the unit's setup label before giving it away.

namespace {
WebServer server(80);
String deviceSuffix;
String deviceName;
String hostName;
String setupSSID;
String savedSSID;
String savedPassword;
String setupWifiPass;
bool accessPointActive = false;
bool mdnsActive = false;
bool httpStarted = false;
bool connectedLogged = false;
volatile uint8_t wifiDisconnectReason = 0;
volatile bool wifiReasonChanged = false;
bool wifiAttemptActive = false;
bool setupSessionActive = false;
unsigned long lastSetupActivityAt = 0;
constexpr unsigned long SETUP_PAUSE_MS = 600000UL; // 10 minutes to select/configure Wi-Fi
unsigned long wifiAttemptStarted = 0;
unsigned long nextWiFiAttemptAt = 0;
unsigned long rebootAt = 0;
constexpr unsigned long WIFI_TIMEOUT_MS = 18000;
constexpr unsigned long RETRY_INTERVAL_MS = 15000;
constexpr unsigned long WIFI_RESET_HOLD_MS = 8000;

const char CONTROL_PAGE[] PROGMEM = R"html(
<!doctype html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tech 49 Lab Touch Light</title>
<style>
:root { color-scheme: dark; }
body { font-family: -apple-system, BlinkMacSystemFont, Arial, sans-serif;
       max-width: 520px; margin: 22px auto; padding: 0 18px;
       background: #0e1725; color: #f3f7ff; text-align:center; }
.card { background: #1b2b40; padding: 22px; border-radius: 18px;
        box-shadow:0 5px 20px #050b1477; }
h1 { color: #7dd3fc; font-size: 25px; margin: 8px 0; }
h2 { font-size:18px; font-weight:500; }
button { color:white; border:0; border-radius:12px; padding:16px 24px;
         font-size:18px; margin:8px; min-width:110px; cursor:pointer;}
.on { background:#087b4a; }.off { background:#ae3434; }
input[type=range]{ width:95%; accent-color:#38bdf8; }
small { color:#abc2d8; }
a{color:#7dd3fc;}
/* The X cannot dismiss an iPhone Home Screen web app; explain how to exit. */
.mainHeader{position:relative;padding-top:5px}
.exitButton{position:absolute;right:-24px;top:-24px;min-width:72px;
min-height:44px;padding:9px;background:#334e6b;color:#fff;
font:600 15px -apple-system,Arial,sans-serif;touch-action:manipulation}
.exitButton:active{background:#526983}
.exitHelp{position:fixed;inset:0;background:#040c18e8;
display:none;align-items:center;justify-content:center;
z-index:50;padding:20px}
.exitHelp.open{display:flex}
.exitCard{max-width:390px;width:100%;border-radius:18px;
background:#1b2b40;padding:24px;box-shadow:0 10px 30px #0009}
.exitCard button{background:#2563a6;min-height:52px}
/* Large, single-tap navigation targets for iPhone / Home Screen mode. */
.navButton{display:block;width:100%;box-sizing:border-box;
background:#2563a6;color:#fff;border:0;border-radius:12px;
padding:16px 12px;min-height:52px;margin:16px 0 0;
font:600 17px -apple-system,BlinkMacSystemFont,Arial,sans-serif;
cursor:pointer;touch-action:manipulation;
-webkit-tap-highlight-color:transparent;}
.navButton:active{background:#1a4d88;}
.navButton:focus-visible{outline:3px solid #7dd3fc;outline-offset:3px;}

/* Tech 49 Lab spacecraft console: local assets only. */
*{box-sizing:border-box}body{background:#080e18;color:#edf5ff;max-width:700px;margin:20px auto;padding:0 16px;font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif}
body:before{content:"";display:block;height:14px;background:linear-gradient(90deg,#378bdf 0 32%,#8b9aad 32% 45%,#2866aa 45% 83%,#aab8c9 83%);border-radius:22px 22px 2px 2px;margin-bottom:14px;background-size:200% 100%;animation:consoleChase 4s linear infinite}
@keyframes consoleChase{from{background-position:200% 0}to{background-position:0 0}}
@media(prefers-reduced-motion:reduce){body:before{animation:none}}
.card{background:linear-gradient(135deg,#182638,#101a29);border:1px solid #435e7c;border-left:9px solid #348cdd;border-radius:30px 10px 10px 10px;padding:22px;margin:14px 0;box-shadow:0 8px 26px #0004}
h1,h2,h3{letter-spacing:.035em}h1{color:#83c8ff}h2{font-size:19px}small{color:#b5c7dc}a{color:#91d0ff}
button,.navButton,.diagNavButton{background:#235d97;color:#f5f9ff;border:1px solid #568ac0;min-height:50px;border-radius:10px;font-weight:650;touch-action:manipulation}
button:focus-visible,input:focus-visible,summary:focus-visible{outline:3px solid #a3d9ff;outline-offset:3px}button:disabled{opacity:.55}
input:not([type=checkbox]):not([type=range]),select{width:100%;min-width:0;background:#0c1624;color:#edf5ff;border:1px solid #6287ad;border-radius:9px;padding:12px;font:inherit}
input[type=range]{width:100%;height:44px;accent-color:#77bfff}
.mainHeader{padding:6px 75px 8px 0;text-align:left}.mainHeader small{letter-spacing:.18em}.exitButton{right:0;top:0;min-width:65px;padding:8px;margin:0;background:#34485e}
.consoleReadout{display:flex;justify-content:space-between;align-items:end;gap:14px;border-top:1px solid #44627e;padding-top:18px;margin-top:16px;text-align:left}.consoleReadout strong{font-size:clamp(40px,13vw,68px);font-variant-numeric:tabular-nums;color:#b3dcff;line-height:1}.consoleReadout small{display:block;font-size:12px;letter-spacing:.1em}
.powerControls{display:flex;gap:12px}.powerControls button{flex:1;margin:10px 0;min-width:0;font-size:21px;min-height:65px}.on{background:#5d2934;border-color:#bb6574}.on[aria-pressed=true]{background:#b52942;box-shadow:0 0 20px #f43f5e44}.off{background:#224b3c;border-color:#519c79}.off[aria-pressed=true]{background:#176a45;box-shadow:0 0 20px #22c55e22}
.segments{height:17px;border-radius:4px;background:repeating-linear-gradient(90deg,transparent 0 calc(10% - 4px),#101a29 calc(10% - 4px) 10%),linear-gradient(90deg,#77bfff var(--fill,0%),#33475d var(--fill,0%));margin:18px 0 8px}
.telemetry{display:flex;flex-wrap:wrap;gap:8px;margin:15px 0}.telemetry span{flex:1;padding:11px;background:#101c2c;border:1px solid #4b6480;border-radius:9px;font-size:13px}
details{text-align:left;margin-top:20px;border-top:2px solid #537492;padding-top:15px}summary{cursor:pointer;color:#add9ff;font-weight:700;letter-spacing:.1em;min-height:44px;padding:8px 0}.phonePanel{margin-top:16px;padding-top:16px;border-top:1px solid #44627e}.phonePanel label{display:block;margin-bottom:9px;font-weight:650}.phonePanel button{margin-left:0}.phonePanel small{display:block;line-height:1.5}.phonePanel p{min-height:1.2em;font-size:14px}
.row{flex-wrap:wrap;overflow-wrap:anywhere}.row span:last-child{max-width:65%}.row span:first-child{flex:1}
@media(max-width:390px){body{padding:0 10px}.card{padding:16px;border-left-width:6px}.row span:last-child{max-width:60%}}

</style></head>
<body><div class="card">
<div class="mainHeader">
<small>TECH 49 LAB</small><h1>Touch Light</h1>
<button type="button" class="exitButton" onclick="showExitHelp()"
aria-label="Exit help">&#10005; Exit</button>
</div>
<h2 id="name">Connecting...</h2>
<div class="telemetry"><span id="wifiStatus">Wi-Fi: checking</span><span id="mqttStatus">MQTT: checking</span></div>
<div class="consoleReadout"><div><small>BRIGHTNESS PRESET</small><h2 id="status" aria-live="polite">Checking light...</h2></div><strong><span id="percent">--</span>%</strong></div>
<div class="segments" id="segments" aria-hidden="true"></div><div class="powerControls">
<button id="powerOn" aria-pressed="false" class="on" onclick="setPower('on')">ON</button>
<button id="powerOff" aria-pressed="false" class="off" onclick="setPower('off')">OFF</button>
</div><h2>Brightness preset: <span id="level">--</span> / 255</h2>
<input aria-label="Brightness preset" type="range" id="brightness" min="5" max="255" value="180"
       oninput="showBrightness(this.value)" onchange="setBrightness(this.value)">
<p><small id="network">Local network control</small></p>
<details><summary>ENGINEERING</summary><small>Setup, diagnostics, reboot and recovery information.</small><p id="setupLink" style="display:none"><a href="/setup">Set up home Wi-Fi</a></p>
<button type="button" class="navButton"
onclick="window.location.assign('/diagnostics')"
aria-label="Open system diagnostics and live log">
System Diagnostics &amp; Live Log &#8594;
</button>
<div class="phonePanel"><label for="phoneName">Name this phone or browser</label>
<input id="phoneName" maxlength="48" placeholder="e.g. Hylan’s iPhone" autocomplete="off">
<button type="button" onclick="savePhoneName()">Save name</button>
<small>Optional label for web log entries; it does not verify a person's identity. Stored in this browser for this address. A different browser, address or cleared website data may need naming again. Home Assistant keeps its own source label.</small>
<p id="phoneMessage" role="status"></p></div></details>
</div>
<p><small>Works without Home Assistant or internet. Use Safari &rarr;
Share &rarr; Add to Home Screen to make an iPhone shortcut.</small></p>
<div class="exitHelp" id="exitHelp" role="dialog" aria-modal="true"
aria-labelledby="exitHelpTitle">
  <div class="exitCard">
    <h2 id="exitHelpTitle">Exit Touch Light</h2>
    <p>The iPhone doesn't let this webpage close itself.
    Swipe up from the bottom of your iPhone to return to the Home Screen.
    In Safari, you can close the tab.</p>
    <button type="button" onclick="hideExitHelp()">Return to Lamp</button>
  </div>
</div>
<script>

const PHONE_NAME_KEY='tech49.browserName.v1';
let browserName='';
try{browserName=localStorage.getItem(PHONE_NAME_KEY)||'';}catch(e){}
document.getElementById('phoneName').value=browserName;
function savePhoneName(){
 const name=document.getElementById('phoneName').value.trim();
 if(new TextEncoder().encode(name).length>48 || /[\x00-\x1f\x7f\[\]]/.test(name)){
  document.getElementById('phoneMessage').textContent='Use up to 48 UTF-8 bytes, without brackets or control characters.';return;
 }
 browserName=name;
 try{localStorage.setItem(PHONE_NAME_KEY,name);document.getElementById('phoneMessage').textContent=name?'Saved for this browser.':'Name cleared; commands will show WEB.';}
 catch(e){document.getElementById('phoneMessage').textContent='Used for this page only; browser storage is unavailable.';}
}
let sliderEditing=false;
let lastSliderTime=0;
const level=document.getElementById('level');
const slider=document.getElementById('brightness');
function updateMeter(n){document.getElementById('percent').textContent=Math.round(Number(n)*100/255);document.getElementById('segments').style.setProperty('--fill',Number(n)*100/255+'%');}
function showBrightness(n){sliderEditing=true;lastSliderTime=Date.now();level.textContent=n;updateMeter(n);}
function showExitHelp(){
  document.getElementById('exitHelp').classList.add('open');
}
function hideExitHelp(){
  document.getElementById('exitHelp').classList.remove('open');
}
async function send(body){
  try { body.clientName=browserName; const response=await fetch('/api/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
                    body:new URLSearchParams(body)}); if(!response.ok)throw Error("Command rejected"); await refresh(); }
  catch(e){document.getElementById('status').textContent='Connection lost';}
}
function setPower(v){send({power:v});}
function setBrightness(v){sliderEditing=false;send({brightness:v});}
async function refresh(){
  try {
    const resp=await fetch('/api/state',{cache:'no-store'});
    if(!resp.ok) throw new Error('HTTP '+resp.status);
    const s=await resp.json();
    document.getElementById('name').textContent=s.name;
    document.getElementById('status').textContent=s.power?'LIGHT ON':'LIGHT OFF — preset';
    document.getElementById('powerOn').setAttribute('aria-pressed',String(s.power));
    document.getElementById('powerOff').setAttribute('aria-pressed',String(!s.power));
    document.getElementById('wifiStatus').textContent=s.mode+': '+(s.wifi?'Connected':(s.setup?'Direct connection':'Disconnected'));
    document.getElementById('mqttStatus').textContent='MQTT: '+(s.mqttEnabled?(s.mqttConnected?'Connected':'Disconnected'):'Disabled');
    if(!sliderEditing || Date.now()-lastSliderTime>2500){
      sliderEditing=false; slider.value=s.brightness;level.textContent=s.brightness;updateMeter(s.brightness);
    }
    document.getElementById('network').textContent=s.mode+'  |  '+s.ip;
    document.getElementById('setupLink').style.display=s.setup?'block':'none';
  } catch(e){ document.getElementById('status').textContent='Disconnected - retrying...'; }
}
refresh();setInterval(refresh,1500);
</script></body></html>
)html";

bool requestFromSetupAP()
{
  return accessPointActive && server.client().localIP() == WiFi.softAPIP();
}

void sendControl()
{
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", CONTROL_PAGE);
}


void sendDiagnosticsPage()
{
  server.sendHeader("Cache-Control","no-store");
  server.send_P(200,"text/html; charset=utf-8",DIAGNOSTICS_PAGE);
}

void sendDiagnostics()
{
  const bool connected=WiFi.status()==WL_CONNECTED;
  String json="{\"device\":\"";
  json.reserve(5000);
  json+=deviceName;
  json+="\",\"firmware\":\"Universal V1.2.3 Console (test)\",\"uptime\":";
  json+=String(millis());
  json+=",\"wifi\":";
  json+=(connected?"true":"false");
  json+=",\"rssi\":";
  json+=String(connected?WiFi.RSSI():0);
  json+=",\"ip\":\"";
  json+=(connected?WiFi.localIP().toString():WiFi.softAPIP().toString());
  json+="\",\"ap\":";
  json+=(accessPointActive?"true":"false");
  json+=",\"mqttEnabled\":";
  json+=(optionalMqttEnabled()?"true":"false");
  json+=",\"mqttConnected\":";
  json+=(optionalMqttConnected()?"true":"false");
  json+=",\"events\":";
  json+=diagEventsJson();
  // Reset information is read from the ESP-IDF reset reason and persistent
  // counters; current iPhone time estimates the *most recent boot* only.
  json+=",\"resetCause\":\"";
  json+=resetMonitorLastCause();
  json+="\",\"resetBoots\":";
  json+=String(resetMonitorBootCount());
  json+=",\"resetWatchdog\":";
  json+=String(resetMonitorWatchdogCount());
  json+=",\"resetWeb\":";
  json+=String(resetMonitorWebRestartCount());
  json+=resetMonitorTestCountsJson();
  json+=",\"watchdogFeeds\":";
  json+=String(resetMonitorFeedCount());
  json+=",\"watchdogActive\":";
  json+=(resetMonitorWatchdogArmed()?"true":"false");
  json+=",\"uptimeSec\":";
  json+=String(static_cast<uint32_t>(esp_timer_get_time()/1000000LL));
  json+="}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",json);
}

void clearDiagnosticsLog()
{
  // Only clear the volatile ring buffer, not Wi-Fi/MQTT settings.
  diagClearEvents();
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json","{\"cleared\":true}");
}

void requestWebRestart()
{
  if (rebootAt != 0) {
    server.send(409,"application/json","{\"error\":\"restart already scheduled\"}");
    return;
  }
  // Reboot only on explicit POST after saving intent to nonvolatile memory.
  // On the next boot the ordinary ESP_RST_SW cause plus this marker is
  // classified as "Web-requested" and counted in reset history.
  if (!resetMonitorMarkWebRestart()) {
    diagLog("[RESET] ERROR: Could not save web restart marker");
    server.send(503,"application/json","{\"error\":\"could not record restart\"}");
    return;
  }
  diagLog("[RESET] Web restart requested (will reboot in 2 seconds)");
  rebootAt = millis() + 2000UL;
  if (rebootAt == 0) rebootAt = 1;
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json","{\"accepted\":true}");
}

void beginSecretReveal()
{
  String payload="{\"token\":\"";
  payload+=diagStartReveal();
  payload+="\"}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",payload);
}

void secretRevealStatus()
{
  String payload="{\"state\":\"";
  payload+=diagRevealStatus(server.arg("token"));
  payload+="\"}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",payload);
}

void secretRevealConsume()
{
  if(!diagConsumeReveal(server.arg("token"))) {
    server.send(403,"application/json",
                "{\"error\":\"physical confirmation required\"}");
    return;
  }
  String payload="{\"ssid\":\"";
  payload+=setupSSID;
  payload+="\",\"password\":\"";
  payload+=setupWifiPass;
  payload+="\"}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",payload);
}

void sendState()
{
  bool powerOn = false;
  uint8_t brightness = 0;
  webControlGetState(powerOn, brightness);
  const bool apRequest = requestFromSetupAP();
  String localIp = apRequest ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  if (WiFi.status() != WL_CONNECTED && !apRequest)
    localIp = WiFi.softAPIP().toString();

  String json = "{\"power\":";
  json += powerOn ? "true" : "false";
  json += ",\"brightness\":";
  json += String(brightness);
  json += ",\"name\":\"";
  json += deviceName;
  json += "\",\"mode\":\"";
  json += apRequest ? "Direct Wi-Fi" : "Home Wi-Fi";
  json += "\",\"ip\":\"";
  json += localIp;
  json += "\",\"setup\":";
  json += apRequest ? "true" : "false";
  json += ",\"mqttEnabled\":";
  json += optionalMqttEnabled() ? "true" : "false";
  json += ",\"mqttConnected\":";
  json += optionalMqttConnected() ? "true" : "false";
  json += ",\"wifi\":";
  json += WiFi.status()==WL_CONNECTED ? "true" : "false";
  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// A browser-supplied label is a convenience, never authentication.
bool browserLabel(String &label)
{
  label=server.arg("clientName");
  label.trim();
  if(label.length()>48) return false;
  for(size_t i=0;i<label.length();++i) {
    const uint8_t c=static_cast<uint8_t>(label[i]);
    if(c<0x20 || c==0x7f || c=='[' || c==']') return false;
  }
  return true;
}

void setLightFromBrowser()
{
  String label;
  if(!browserLabel(label)) {
    server.send(400,"text/plain","Invalid browser name (maximum 48 bytes; no brackets or controls)");
    return;
  }
  bool powerValid=false, powerOn=false, brightnessValid=false;
  uint8_t requestedBrightness=0;
  if (server.hasArg("power")) {
    const String value=server.arg("power");
    if (value!="on" && value!="off") {
      server.send(400, "text/plain", "power must be on or off");
      return;
    }
    powerValid=true;
    powerOn=value=="on";
  }
  if (server.hasArg("brightness")) {
    const String value=server.arg("brightness");
    for(size_t i=0;i<value.length();++i) {
      if(value[i]<'0'||value[i]>'9') {
        server.send(400, "text/plain", "Invalid brightness");
        return;
      }
    }
    int v=value.toInt();
    if(value.length()==0 || v<5 || v>255) {
      server.send(400, "text/plain", "Brightness must be 5 to 255");
      return;
    }
    brightnessValid=true;
    requestedBrightness=static_cast<uint8_t>(v);
  }
  if (!powerValid && !brightnessValid) {
    server.send(400, "text/plain", "No control provided");
    return;
  }
  webControlCommand(powerValid,powerOn,brightnessValid,requestedBrightness,label.c_str());
  sendState();
}

void beginSetupSession()
{
  lastSetupActivityAt=millis();
  if(!setupSessionActive) {
    setupSessionActive=true;
    // Prevent a station connection attempt from interrupting Wi-Fi scanning.
    // This does NOT erase the saved SSID/password in Preferences.
    WiFi.disconnect(false,false);
    wifiAttemptActive=false;
    Serial.println("Wi-Fi setup: paused automatic home-network retries.");
  }
}

// Escape a Wi-Fi SSID so even unusual names are valid JSON strings.
String escapeJson(const String &value)
{
  String result;
  result.reserve(value.length()+8);
  for(size_t i=0;i<value.length();i++) {
    const uint8_t c=static_cast<uint8_t>(value[i]);
    if(c=='"' || c=='\\') {
      result+='\\';
      result+=static_cast<char>(c);
    } else if(c<0x20) {
      char hex[7];
      snprintf(hex,sizeof(hex),"\\u%04x",static_cast<unsigned int>(c));
      result+=hex;
    } else {
      result+=static_cast<char>(c);
    }
  }
  return result;
}

void sendNearbyNetworks()
{
  if(!requestFromSetupAP()) {
    server.send(403,"application/json","{\"state\":\"error\"}");
    return;
  }
  beginSetupSession();

  const int16_t count=WiFi.scanComplete();
  if(count == WIFI_SCAN_FAILED) {
    // Async scan; the browser polls this endpoint until results are ready.
    const int16_t started=WiFi.scanNetworks(true,true);
    if(started == WIFI_SCAN_FAILED) {
      server.send(503,"application/json","{\"state\":\"error\"}");
      return;
    }
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json","{\"state\":\"scanning\"}");
    return;
  }
  if(count == WIFI_SCAN_RUNNING) {
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json","{\"state\":\"scanning\"}");
    return;
  }

  String result="{\"state\":\"ready\",\"networks\":[";
  result.reserve(3200);
  // Duplicate SSIDs can appear from different APs. Keep only the first one.
  String listed[30];
  int found=0;
  for(int16_t i=0;i<count && found<30;i++) {
    const String ssid=WiFi.SSID(i);
    if(ssid.isEmpty()) continue;
    bool duplicate=false;
    for(int j=0;j<found;j++) {
      if(listed[j]==ssid) {duplicate=true;break;}
    }
    if(duplicate) continue;
    listed[found]=ssid;
    if(found++>0)result+=",";
    result+="{\"ssid\":\"";
    result+=escapeJson(ssid);
    result+="\",\"rssi\":";
    result+=String(WiFi.RSSI(i));
    result+=",\"secure\":";
    result+=(WiFi.encryptionType(i)==WIFI_AUTH_OPEN ? "false" : "true");
    result+="}";
  }
  result+="]}";
  WiFi.scanDelete();
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",result);
}

void sendSetup()
{
  if(!requestFromSetupAP()) {
    server.send(403,"text/plain","Setup only available through the light's temporary Wi-Fi network.");
    return;
  }
  beginSetupSession();
  String html=R"html(<!doctype html><html><head><meta name="viewport"
content="width=device-width,initial-scale=1"><title>Wi-Fi setup</title>
<style>body{background:#101b2b;color:white;font:18px -apple-system,Arial,sans-serif;
max-width:420px;margin:32px auto;padding:0 15px;}input,button{display:block;width:95%;
font:18px Arial;padding:12px;margin:14px 0;border-radius:8px}button{background:#148359;color:#fff;border:0}
a{color:#7dd3fc}
/* Tech 49 Lab spacecraft console: local assets only. */
*{box-sizing:border-box}body{background:#080e18;color:#edf5ff;max-width:700px;margin:20px auto;padding:0 16px;font-family:-apple-system,BlinkMacSystemFont,Arial,sans-serif}
body:before{content:"";display:block;height:14px;background:linear-gradient(90deg,#378bdf 0 32%,#8b9aad 32% 45%,#2866aa 45% 83%,#aab8c9 83%);border-radius:22px 22px 2px 2px;margin-bottom:14px;background-size:200% 100%;animation:consoleChase 4s linear infinite}
@keyframes consoleChase{from{background-position:200% 0}to{background-position:0 0}}
@media(prefers-reduced-motion:reduce){body:before{animation:none}}
.card{background:linear-gradient(135deg,#182638,#101a29);border:1px solid #435e7c;border-left:9px solid #348cdd;border-radius:30px 10px 10px 10px;padding:22px;margin:14px 0;box-shadow:0 8px 26px #0004}
h1,h2,h3{letter-spacing:.035em}h1{color:#83c8ff}h2{font-size:19px}small{color:#b5c7dc}a{color:#91d0ff}
button,.navButton,.diagNavButton{background:#235d97;color:#f5f9ff;border:1px solid #568ac0;min-height:50px;border-radius:10px;font-weight:650;touch-action:manipulation}
button:focus-visible,input:focus-visible,summary:focus-visible{outline:3px solid #a3d9ff;outline-offset:3px}button:disabled{opacity:.55}
input:not([type=checkbox]):not([type=range]),select{width:100%;min-width:0;background:#0c1624;color:#edf5ff;border:1px solid #6287ad;border-radius:9px;padding:12px;font:inherit}
input[type=range]{width:100%;height:44px;accent-color:#77bfff}
.mainHeader{padding:6px 75px 8px 0;text-align:left}.mainHeader small{letter-spacing:.18em}.exitButton{right:0;top:0;min-width:65px;padding:8px;margin:0;background:#34485e}
.consoleReadout{display:flex;justify-content:space-between;align-items:end;gap:14px;border-top:1px solid #44627e;padding-top:18px;margin-top:16px;text-align:left}.consoleReadout strong{font-size:clamp(40px,13vw,68px);font-variant-numeric:tabular-nums;color:#b3dcff;line-height:1}.consoleReadout small{display:block;font-size:12px;letter-spacing:.1em}
.powerControls{display:flex;gap:12px}.powerControls button{flex:1;margin:10px 0;min-width:0;font-size:21px;min-height:65px}.on{background:#5d2934;border-color:#bb6574}.on[aria-pressed=true]{background:#b52942;box-shadow:0 0 20px #f43f5e44}.off{background:#224b3c;border-color:#519c79}.off[aria-pressed=true]{background:#176a45;box-shadow:0 0 20px #22c55e22}
.segments{height:17px;border-radius:4px;background:repeating-linear-gradient(90deg,transparent 0 calc(10% - 4px),#101a29 calc(10% - 4px) 10%),linear-gradient(90deg,#77bfff var(--fill,0%),#33475d var(--fill,0%));margin:18px 0 8px}
.telemetry{display:flex;flex-wrap:wrap;gap:8px;margin:15px 0}.telemetry span{flex:1;padding:11px;background:#101c2c;border:1px solid #4b6480;border-radius:9px;font-size:13px}
details{text-align:left;margin-top:20px;border-top:2px solid #537492;padding-top:15px}summary{cursor:pointer;color:#add9ff;font-weight:700;letter-spacing:.1em;min-height:44px;padding:8px 0}.phonePanel{margin-top:16px;padding-top:16px;border-top:1px solid #44627e}.phonePanel label{display:block;margin-bottom:9px;font-weight:650}.phonePanel button{margin-left:0}.phonePanel small{display:block;line-height:1.5}.phonePanel p{min-height:1.2em;font-size:14px}
.row{flex-wrap:wrap;overflow-wrap:anywhere}.row span:last-child{max-width:65%}.row span:first-child{flex:1}
@media(max-width:390px){body{padding:0 10px}.card{padding:16px;border-left-width:6px}.row span:last-child{max-width:60%}}
</style></head><body>
<h2>Tech 49 Lab — Wi-Fi setup</h2>
<p>Select your home's Wi-Fi network from the list, then enter its password.</p>
<label>Nearby 2.4 GHz Wi-Fi networks
<select id="networks" onchange="chooseNetwork()" style="display:block;width:100%;font-size:17px;padding:12px;margin:14px 0;border-radius:8px;">
<option value="">Scanning nearby networks...</option></select></label>
<button type="button" onclick="refreshNetworks()" style="background:#2266a3;">Refresh network list</button>
<p id="scanStatus" aria-live="polite">Checking nearby networks...</p>
<form method="post" action="/setup">
<label>Selected network name (SSID)
<input id="ssid" name="ssid" maxlength="32" placeholder="Select above or type name" required></label>
<label>Home Wi-Fi password
<input name="password" type="password" maxlength="63" autocomplete="new-password"></label>
<div style="background:#1a3045;padding:12px;border-radius:12px;margin-top:18px;">
<h3>Optional smart-home connection</h3>
<label style="display:block;margin:12px 0">
<input type="checkbox" name="mqtt" value="1" id="mqtt" onchange="toggleMqtt()"
style="width:auto;display:inline;margin-right:6px;">
Use Home Assistant (MQTT)</label>
<p style="font-size:15px;color:#c9d9ec">Leave this unchecked if you only want
iPhone browser and touch control. Home Assistant requires an MQTT broker.</p>
<div id="mqttDetails" hidden>
<label>MQTT broker address (e.g. 192.168.1.218)
<input id="mqttHost" name="mqttHost" maxlength="100"
placeholder="Broker IP or hostname"></label>
<label>MQTT port
<input name="mqttPort" type="number" min="1" max="65535" value="1883"></label>
<label>MQTT username (if required)
<input name="mqttUser" maxlength="64" autocomplete="off"></label>
<label>MQTT password (if required)
<input name="mqttPass" type="password" maxlength="128" autocomplete="new-password"></label>
</div></div>
<button type="submit">Save &amp; connect</button></form>
<p><small>The ESP32-C3 supports 2.4 GHz Wi-Fi. If your network is hidden or
missing from the list, enter the exact network name manually.
The lamp restarts after saving. The touch pads work without Wi-Fi.</small></p>
<p><a href="/">Back to light controls</a></p>
<script>
function toggleMqtt(){
  const enabled=document.getElementById('mqtt').checked;
  document.getElementById('mqttDetails').hidden=!enabled;
  document.getElementById('mqttHost').required=enabled;
}
function chooseNetwork() {
  const selected=document.getElementById('networks').value;
  if(selected)document.getElementById('ssid').value=selected;
}
function showScanMessage(message){
  document.getElementById('scanStatus').textContent=message;
}
async function refreshNetworks(){
  const select=document.getElementById('networks');
  select.replaceChildren(new Option('Scanning 2.4 GHz networks...', ''));
  showScanMessage('Scanning nearby networks. This can take a few seconds...');
  try {
    for(let attempt=0;attempt<20;attempt++){
      const response=await fetch('/api/networks',{cache:'no-store'});
      if(!response.ok)throw new Error('Scan not available');
      const data=await response.json();
      if(data.state==='scanning'){
        await new Promise(resolve=>setTimeout(resolve,800));
        continue;
      }
      if(data.state!=='ready')throw new Error('Wi-Fi scan failed');
      select.replaceChildren(new Option('Choose your network...', ''));
      const networks=data.networks || [];
      networks.sort((a,b)=>b.rssi-a.rssi);
      networks.forEach(n=>{
        const label=n.ssid+' ('+n.rssi+' dBm, '+(n.secure?'secured':'open')+')';
        select.add(new Option(label,n.ssid));
      });
      showScanMessage(networks.length+
        ' networks found. Select yours above, or enter the name manually.');
      return;
    }
    throw new Error('Scan timed out');
  }catch(err) {
    select.replaceChildren(new Option('Enter your network name manually', ''));
    showScanMessage('Network scan unavailable. Type your network name below.');
  }
}
refreshNetworks();
</script></body></html>)html";
  server.send(200,"text/html; charset=utf-8",html);
}

void saveSetup()
{
  if(!requestFromSetupAP()) {
    server.send(403,"text/plain","Only available via setup Wi-Fi");
    return;
  }
  const String ssid=server.arg("ssid");
  const String password=server.arg("password");
  if(ssid.length()<1 || ssid.length()>32 ||
     (password.length()>0 && (password.length()<8 || password.length()>63))) {
    server.send(400,"text/plain","SSID or WPA2 password length invalid");
    return;
  }
  // Only provision MQTT when the owner explicitly opts in.
  const bool mqttEnabled=server.hasArg("mqtt");
  const String mqttHost=server.arg("mqttHost");
  const String mqttUser=server.arg("mqttUser");
  const String mqttPass=server.arg("mqttPass");
  const String portText=server.arg("mqttPort");
  uint16_t mqttPort=1883;
  if(mqttEnabled) {
    if(mqttHost.isEmpty() || mqttHost.length()>100 ||
       mqttUser.length()>64 || mqttPass.length()>128 ||
       portText.isEmpty() || portText.length()>5) {
      server.send(400,"text/plain","Invalid MQTT broker settings");
      return;
    }
    for(size_t i=0;i<portText.length();++i) {
      if(portText[i]<'0'||portText[i]>'9') {
        server.send(400,"text/plain","Invalid MQTT port");
        return;
      }
    }
    const unsigned long p=portText.toInt();
    if(p<1 || p>65535) {
      server.send(400,"text/plain","MQTT port must be 1 to 65535");
      return;
    }
    mqttPort=static_cast<uint16_t>(p);
  }
  if(!optionalMqttSaveSettings(mqttEnabled,mqttHost,mqttPort,mqttUser,mqttPass)) {
    server.send(500,"text/plain","Cannot store optional MQTT settings");
    return;
  }

  Preferences prefs;
  if(!prefs.begin("tech49light",false)) {
    server.send(500,"text/plain","Cannot open NVS preferences");
    return;
  }
  prefs.putString("ssid",ssid);
  prefs.putString("password",password);
  prefs.end();

  server.send(200,"text/html; charset=utf-8",
              "<html><meta name='viewport' content='width=device-width'>"
              "<h2>Saved! The light is restarting.</h2><p>Reconnect your "
              "iPhone to your home Wi-Fi and open the light's local address.</p></html>");
  rebootAt=millis()+1800;
}

void startAP()
{
  if (accessPointActive) return;
  WiFi.mode(WIFI_AP_STA);
  accessPointActive=WiFi.softAP(setupSSID.c_str(),setupWifiPass.c_str());
  if (accessPointActive) {
    Serial.print("Setup/direct Wi-Fi: "); Serial.println(setupSSID);
    Serial.print("Setup Wi-Fi password (print on unit label): ");
    Serial.println(setupWifiPass);
    Serial.print("Setup IP: "); Serial.println(WiFi.softAPIP());
    diagLog("[WIFI] Direct setup Wi-Fi access point activated");
    Serial.println("Safari: http://192.168.4.1");
  } else Serial.println("ERROR: could not start setup AP.");
}

void startStationAttempt()
{
  // Never call WiFi.reconnect() while an ESP32 station connection is still
  // in progress. It otherwise emits "wifi:sta is connecting, return error".
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  wifiAttemptStarted=millis();
  wifiAttemptActive=true;
  Serial.println("Wi-Fi: attempting to join saved 2.4 GHz network...");
  diagLog("[WIFI] Connecting to saved household Wi-Fi");
}

void beginStation()
{
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostName.c_str());
  // A single explicit state machine controls retries, not auto-reconnect
  // AND a second periodic reconnect competing with the first attempt.
  WiFi.setAutoReconnect(false);
  connectedLogged=false;
  wifiAttemptActive=false;
  startStationAttempt();
}

void checkWiFi()
{
  const unsigned long now=millis();
  const wl_status_t status=WiFi.status();

  if(setupSessionActive) {
    if(now-lastSetupActivityAt<SETUP_PAUSE_MS) return;
    setupSessionActive=false;
    wifiAttemptActive=false;
    nextWiFiAttemptAt=now+RETRY_INTERVAL_MS;
    Serial.println("Wi-Fi setup inactive; resuming home-network retries.");
  }

  if(status==WL_CONNECTED) {
    wifiAttemptActive=false;
    if(!connectedLogged){
      connectedLogged=true;
      Serial.print("Wi-Fi connected. IP: "); Serial.println(WiFi.localIP());
      diagLogf("[WIFI] Connected, IP %s, RSSI %d dBm",
               WiFi.localIP().toString().c_str(),WiFi.RSSI());
      if(accessPointActive){
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        accessPointActive=false;
      }
    }
    if(!mdnsActive){
      mdnsActive=MDNS.begin(hostName.c_str());
      if(mdnsActive) MDNS.addService("http","tcp",80);
      Serial.print("iPhone Safari address: http://");Serial.print(hostName);
      Serial.println(".local");
    }
    return;
  }

  if(connectedLogged){
    connectedLogged=false;
    if(mdnsActive){MDNS.end();mdnsActive=false;}
    wifiAttemptActive=false;
    nextWiFiAttemptAt=now+RETRY_INTERVAL_MS;
    Serial.println("Wi-Fi: disconnected; touch still works; retry scheduled.");
    diagLog("[WIFI] Disconnected; scheduled reconnection");
    if(!accessPointActive)startAP();
  }

  // Allow a full association/authentication attempt to complete. Do not
  // attempt another join until the current one has timed out or failed.
  if(wifiAttemptActive){
    if(now-wifiAttemptStarted<WIFI_TIMEOUT_MS) return;

    Serial.print("Wi-Fi: connection timed out (Arduino status ");
    Serial.print(static_cast<int>(status));
    Serial.println("). Check exact 2.4 GHz SSID, password, and WPA2 settings.");
    diagLogf("[WIFI] Home network join timed out (Arduino status %d)",
             static_cast<int>(status));
    WiFi.disconnect(false,false); // stop attempt; retain credentials in NVS
    wifiAttemptActive=false;
    nextWiFiAttemptAt=now+RETRY_INTERVAL_MS;
    if(!accessPointActive)startAP(); // allows retrying Wi-Fi setup from iPhone
    return;
  }

  if(static_cast<int32_t>(now-nextWiFiAttemptAt)>=0){
    startStationAttempt();
  }
}

bool resetWiFiIfTouchHeld()
{
  // Hold the physical touch pad continuously for 8s DURING POWER-UP to
  // clear the stored household Wi-Fi credentials. Local touch still works.
  delay(250); // allow the touch sensor output to settle
  if(digitalRead(TOUCH_PIN)!=LOW) return false;
  Serial.println("Hold touch for 8 seconds at startup to reset Wi-Fi.");
  unsigned long heldFrom=millis();
  while(millis()-heldFrom < WIFI_RESET_HOLD_MS){
    if(digitalRead(TOUCH_PIN)!=LOW)return false;
    delay(20);
  }
  Preferences prefs;
  if(prefs.begin("tech49light",false)){
    prefs.remove("ssid");
    prefs.remove("password");
    prefs.end();
    optionalMqttEraseSettings();
    diagLog("[SECURITY] Wi-Fi and optional MQTT settings factory-reset");
    Serial.println("Stored Wi-Fi and optional MQTT settings erased.");
  }
  return true;
}
} // namespace

void webControlBegin()
{
  char suffix[7];
  snprintf(suffix,sizeof(suffix),"%06lX",
           static_cast<unsigned long>(ESP.getEfuseMac() & 0xFFFFFFULL));
  deviceSuffix=suffix;
  deviceName="Touch Light "+deviceSuffix;
  hostName="tech49-light-"+deviceSuffix;
  hostName.toLowerCase();
  setupSSID="Tech49-Light-"+deviceSuffix;

  // One random setup password per board, retained across firmware uploads.
  Preferences provision;
  if(provision.begin("tech49setup",false)){
    setupWifiPass=provision.getString("password","");
    if(setupWifiPass.length()<8) {
      char pass[16];
      snprintf(pass,sizeof(pass),"T49-%08lX",
               static_cast<unsigned long>(esp_random()));
      setupWifiPass=pass;
      provision.putString("password",setupWifiPass);
    }
    provision.end();
  }
  if(setupWifiPass.length()<8) {
    Serial.println("ERROR: Failed to store the unit's setup Wi-Fi password.");
    return;
  }

  const bool forceSetup=resetWiFiIfTouchHeld();
  Preferences prefs;
  if(prefs.begin("tech49light",true)){
    savedSSID=prefs.getString("ssid","");
    savedPassword=prefs.getString("password","");
    prefs.end();
  }
  if(forceSetup){savedSSID="";savedPassword="";}

  // Log why router association fails without exposing Wi-Fi passwords.
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if(event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      const uint8_t reason=info.wifi_sta_disconnected.reason;
      wifiDisconnectReason=reason;
      wifiReasonChanged=true;
      Serial.print("Wi-Fi: station disconnect reason = ");
      Serial.println(static_cast<unsigned>(reason));
    }
  });

  Serial.println("Wi-Fi setup: Universal V1.2.3 Console (test).");
  diagLog("[SYSTEM] Firmware V1.2.3 started");
  optionalMqttBegin(deviceSuffix);
  server.on("/",HTTP_GET,sendControl);
  server.on("/diagnostics",HTTP_GET,sendDiagnosticsPage);
  server.on("/api/diagnostics",HTTP_GET,sendDiagnostics);
  server.on("/api/diag/clear",HTTP_POST,clearDiagnosticsLog);
  server.on("/api/diag/restart",HTTP_POST,requestWebRestart);
  server.on("/api/diag/test/reset",HTTP_POST,[](){
    if (!resetMonitorResetTestCounts(server.arg("epoch"))) {
      server.send(503,"application/json","{\"error\":\"test reset not saved\"}"); return;
    }
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json","{\"reset\":true}");
  });
  server.on("/api/diag/reveal/start",HTTP_POST,beginSecretReveal);
  server.on("/api/diag/reveal/status",HTTP_POST,secretRevealStatus);
  server.on("/api/diag/reveal/consume",HTTP_POST,secretRevealConsume);
  server.on("/api/state",HTTP_GET,sendState);
  server.on("/api/set",HTTP_POST,setLightFromBrowser);
  server.on("/setup",HTTP_GET,sendSetup);
  server.on("/api/networks",HTTP_GET,sendNearbyNetworks);
  server.on("/setup",HTTP_POST,saveSetup);
  server.onNotFound([](){
    server.send(404,"text/plain","Page not found. Open /");
  });

  if(savedSSID.length()>0)beginStation();
  else {
    Serial.println("No home Wi-Fi saved. Starting direct/setup Wi-Fi.");
    startAP();
  }
  server.begin();
  httpStarted=true;
  Serial.print("Unit name: "); Serial.println(deviceName);
}

void webControlLoop()
{
  if(wifiReasonChanged) {
    const uint8_t reason=wifiDisconnectReason;
    wifiReasonChanged=false;
    diagLogf("[WIFI] Station disconnect reason %u",
             static_cast<unsigned>(reason));
  }
  if(httpStarted)server.handleClient();
  if(savedSSID.length()>0)checkWiFi();
  optionalMqttLoop();
  if(rebootAt!=0 && static_cast<int32_t>(millis()-rebootAt)>=0)
    ESP.restart();
}

