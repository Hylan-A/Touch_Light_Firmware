#include "WebControl.h"
#include "BoardConfig.h"
#include "DeviceIdentity.h"
#include "OptionalMqtt.h"
#include "Diagnostics.h"
#include "DiagnosticsPage.h"
#include "ResetMonitor.h"
#include "FirmwareIdentity.h"

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
.powerControls{display:flex;gap:12px}.powerControls button{flex:1;margin:10px 0;min-width:0;font-size:21px;min-height:65px}.on{background:#5d2934;border-color:#bb6574}.on[aria-pressed=true]{background:#e01d39;box-shadow:0 0 24px #ff3f60aa,inset 0 2px 2px #ffffff44}.off{background:#224b3c;border-color:#519c79}.off[aria-pressed=true]{background:#0cb65d;box-shadow:0 0 24px #22ff88aa,inset 0 2px 2px #ffffff44}
.segments{height:17px;border-radius:4px;background:repeating-linear-gradient(90deg,transparent 0 calc(10% - 4px),#101a29 calc(10% - 4px) 10%),linear-gradient(90deg,#77bfff var(--fill,0%),#33475d var(--fill,0%));margin:18px 0 8px}
.telemetry{display:flex;flex-wrap:wrap;gap:8px;margin:15px 0}.telemetry{align-items:stretch}.telemetry .setupTile{flex:1 1 210px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;min-width:0;padding:11px;background:#101c2c;border:1px solid #4b6480;border-radius:9px;text-align:center;text-decoration:none;color:#edf5ff;font-size:13px;cursor:pointer}.telemetry .setupTile:hover,.telemetry .setupTile:focus-visible{background:#203b59;border-color:#76b9f2}.telemetry .setupTile:active{background:#235d97}.setupTile .tileHint{font-size:11px;color:#94c9f5}.frontSignal{display:inline-flex;align-items:center;justify-content:center;gap:7px;white-space:nowrap;font-size:12px}.frontSignal .signalBars{flex:none}.frontSignal #frontRssiText{display:inline-block;width:23ch;text-align:left;font-variant-numeric:tabular-nums}.frontSignal .signalBars{display:inline-flex;align-items:flex-end;gap:2px;height:18px}.frontSignal .signalBars i{display:block;width:4px;border-radius:2px 2px 0 0;background:#435367}.frontSignal .signalBars i:nth-child(1){height:5px}.frontSignal .signalBars i:nth-child(2){height:8px}.frontSignal .signalBars i:nth-child(3){height:11px}.frontSignal .signalBars i:nth-child(4){height:14px}.frontSignal .signalBars i:nth-child(5){height:18px}.frontSignal .signalBars i.active{background:currentColor;box-shadow:0 0 5px currentColor}.frontSignal.excellent{color:#4ade80}.frontSignal.good{color:#60a5fa}.frontSignal.marginal{color:#facc15}.frontSignal.weak{color:#fb7185}.frontSignal.offline{color:#94a3b8}
details{text-align:left;margin-top:20px;border-top:2px solid #537492;padding-top:15px}summary{cursor:pointer;color:#add9ff;font-weight:700;letter-spacing:.1em;min-height:44px;padding:8px 0}.phonePanel{margin-top:16px;padding-top:16px;border-top:1px solid #44627e}.phonePanel label{display:block;margin-bottom:9px;font-weight:650}.phonePanel button{margin-left:0}.phonePanel small{display:block;line-height:1.5}.phonePanel p{min-height:1.2em;font-size:14px}
.row{flex-wrap:wrap;overflow-wrap:anywhere}.row span:last-child{max-width:65%}.row span:first-child{flex:1}
@media(max-width:390px){body{padding:0 10px}.card{padding:16px;border-left-width:6px}.row span:last-child{max-width:60%}}


/* Raised console fader; native input remains a full-height touch target. */
#brightness::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:18px;height:32px;border-radius:4px;border:1px solid #a7d7fa;background:repeating-linear-gradient(0deg,transparent 0 6px,#102d4c 6px 8px,transparent 8px 11px),linear-gradient(90deg,#20466c,#d7e9f7 42%,#2e83cc 68%,#123b67);box-shadow:inset 2px 0 3px #fff8,2px 3px 7px #0009,0 0 8px #42a5f5}
#brightness{appearance:none;-webkit-appearance:none;background:linear-gradient(#314b65,#17283b);border-radius:20px}
#brightness::-moz-range-thumb{width:18px;height:32px;border-radius:4px;border:1px solid #a7d7fa;background:linear-gradient(90deg,#20466c,#d7e9f7,#2e83cc,#123b67);box-shadow:2px 3px 7px #0009,0 0 8px #42a5f5}
.engineeringButton{display:block;margin:18px auto 6px;min-width:190px;padding:12px 18px;min-height:48px;font-size:15px;letter-spacing:.09em;color:#a6dcff;background:linear-gradient(#526c86,#223b54);border:1px solid #61b9ff;box-shadow:inset 0 2px 2px #fff5,0 0 10px #2694eb77,0 4px 7px #0008}
.engineeringButton:active{transform:translateY(2px);box-shadow:inset 0 2px 6px #0009,0 0 13px #4bbcff}
</style></head>
<body><div class="card">
<div class="mainHeader">
<small>TECH 49 LAB</small><h1>Touch Light</h1>
<button type="button" class="exitButton" onclick="showExitHelp()"
aria-label="Exit help">&#10005; Exit</button>
</div>
<h2 id="name">Connecting...</h2>
<p class="webAddress"><strong>Web address:</strong><br><a id="webAddress" style="color:#add9ff;overflow-wrap:anywhere">Checking...</a><br><small>Share with another phone on the same home Wi-Fi.</small></p>
<div class="consoleReadout"><div><small>BRIGHTNESS PRESET</small><h2 id="status" aria-live="polite">Checking light...</h2></div><strong><span id="percent">--</span>%</strong></div>
<div class="segments" id="segments" aria-hidden="true"></div><div class="powerControls">
<button id="powerOn" aria-pressed="false" class="on" onclick="setPower('on')">ON</button>
<button id="powerOff" aria-pressed="false" class="off" onclick="setPower('off')">OFF</button>
</div><h2>Brightness preset: <span id="level">--</span> / 255</h2>
<input aria-label="Brightness preset" type="range" id="brightness" min="5" max="255" value="180"
       oninput="showBrightness(this.value)" onchange="setBrightness(this.value)">
<div class="telemetry"><div class="setupTile"><span id="wifiStatus">Home Wi-Fi: checking</span><span id="frontSignal" class="frontSignal offline" aria-label="Wi-Fi signal checking"><span class="signalBars" aria-hidden="true"><i></i><i></i><i></i><i></i><i></i></span><span id="frontRssiText">Checking...</span></span></div><div class="setupTile"><span id="mqttStatus">MQTT: checking</span><span id="mqttBroker">Broker: —</span></div></div>
<p><small id="network">Local network control</small></p>
<button type="button" class="engineeringButton" onclick="window.location.assign('/diagnostics')">⚙ ENGINEERING</button>

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

// Shared browser-local audio preference. A click is a user gesture, so the
// Web Audio oscillator is permitted on modern mobile browsers.
const SOUND_KEY='tech49.buttonSound.v1';
let buttonSound=true;
try{buttonSound=localStorage.getItem(SOUND_KEY)!=='off';}catch(e){}
function playButtonBeep(){
 if(!buttonSound)return;
 try{
  const Ctx=window.AudioContext||window.webkitAudioContext;
  if(!Ctx)return;
  const ctx=window.tech49Audio||(window.tech49Audio=new Ctx());
  if(ctx.state==='suspended')ctx.resume();
  const osc=ctx.createOscillator(),gain=ctx.createGain(),t=ctx.currentTime;
  osc.type='sine';osc.frequency.value=880;
  gain.gain.setValueAtTime(0.0001,t);
  gain.gain.exponentialRampToValueAtTime(0.09,t+0.008);
  gain.gain.exponentialRampToValueAtTime(0.0001,t+0.075);
  osc.connect(gain);gain.connect(ctx.destination);osc.start(t);osc.stop(t+0.078);
 }catch(e){} // Audio must never interrupt the light or diagnostics controls.
}
document.addEventListener('click',e=>{
 const b=e.target.closest('button,summary');
 if(b&&!b.disabled)playButtonBeep();
},true);

const PHONE_NAME_KEY='tech49.browserName.v1';
let browserName='';
try{browserName=localStorage.getItem(PHONE_NAME_KEY)||'';}catch(e){}
let sliderEditing=false;
let pendingBrightness=null;
let pendingBrightnessSince=0;
let brightnessRequestId=0;
let brightnessError=false;
const level=document.getElementById('level');
const slider=document.getElementById('brightness');
function updateMeter(n){document.getElementById('percent').textContent=Math.round(Number(n)*100/255);document.getElementById('segments').style.setProperty('--fill',Number(n)*100/255+'%');}
function showBrightness(n){sliderEditing=true;pendingBrightness=null;brightnessError=false;level.textContent=n;updateMeter(n);}
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
function updateFrontSignal(rssi,connected){
 const valid=connected && Number.isFinite(rssi);
 const rating=!valid?'Not connected':rssi>=-60?'Excellent':rssi>=-70?'Good':rssi>=-80?'Marginal':'Weak';
 const css=!valid?'offline':rssi>=-60?'excellent':rssi>=-70?'good':rssi>=-80?'marginal':'weak';
 const count=!valid?0:rssi>=-60?5:rssi>=-67?4:rssi>=-73?3:rssi>=-80?2:1;
 const el=document.getElementById('frontSignal');el.className='frontSignal '+css;
 el.querySelectorAll('.signalBars i').forEach((bar,i)=>bar.classList.toggle('active',i<count));
 document.getElementById('frontRssiText').textContent=valid?rating+' ('+rssi+' dBm)':rating;
 el.setAttribute('aria-label','Wi-Fi signal: '+rating+(valid?', '+rssi+' dBm':''));
}
function setPower(v){send({power:v});}
async function setBrightness(v){
  sliderEditing=false;
  const target=Number(v);
  const requestId=++brightnessRequestId;
  pendingBrightness=target;
  pendingBrightnessSince=Date.now();
  brightnessError=false;
  try {
    const response=await fetch('/api/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:new URLSearchParams({brightness:String(target),clientName:browserName})});
    if(!response.ok)throw Error('Brightness command rejected');
    if(requestId===brightnessRequestId)await refresh();
  } catch(e){
    if(requestId===brightnessRequestId){
      pendingBrightness=null;
      brightnessError=true;
      document.getElementById('status').textContent='Brightness update failed — check connection';
    }
  }
}
async function refresh(){
  try {
    const resp=await fetch('/api/state',{cache:'no-store'});
    if(!resp.ok) throw new Error('HTTP '+resp.status);
    const s=await resp.json();
    document.getElementById('name').textContent=s.name;
    const address=document.getElementById('webAddress');
    address.textContent=s.homeWebAddress;address.href=s.homeWebAddress;
    if(!brightnessError)document.getElementById('status').textContent=s.power?'LIGHT ON':'LIGHT OFF — preset';
    document.getElementById('powerOn').setAttribute('aria-pressed',String(s.power));
    document.getElementById('powerOff').setAttribute('aria-pressed',String(!s.power));
    document.getElementById('wifiStatus').textContent=s.mode+': '+(s.wifi?'Connected':(s.setup?'Direct connection':'Disconnected'));
    updateFrontSignal(s.rssi,s.wifi);
    document.getElementById('mqttStatus').textContent='MQTT: '+(s.mqttEnabled?(s.mqttConnected?'Connected':'Disconnected'):'Disabled');
    document.getElementById('mqttBroker').textContent='Broker: '+(s.mqttHost||'Not configured');
    // Keep the user-selected slider position until the device reports the new value.
    // Old status requests can arrive after a brightness command; never let them
    // overwrite the new position while the command is pending.
    if(pendingBrightness!==null){
      if(Number(s.brightness)===pendingBrightness){
        pendingBrightness=null;
        brightnessError=false;
      }else if(Date.now()-pendingBrightnessSince>8000){
        pendingBrightness=null;
        brightnessError=true;
        document.getElementById('status').textContent='Brightness not confirmed — check connection';
      }
    }
    if(!sliderEditing && pendingBrightness===null && !brightnessError){
      slider.value=s.brightness;level.textContent=s.brightness;updateMeter(s.brightness);
    }
    document.getElementById('network').textContent=s.mode+'  |  '+s.ip;

  } catch(e){ document.getElementById('status').textContent='Disconnected - retrying...'; }
}
refresh();setInterval(refresh,1000);
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
  json+="\",\"firmware\":\"Universal V1.3.0 Console (test)\"";
  json+=",\"boardId\":\"";json+=BOARD_ID;
  json += "\",\"macAddress\":\""; json += WiFi.macAddress();
  json+="\",\"hardwareRev\":\"";json+=HARDWARE_REV;
  json+="\",\"firmwareType\":\"";json+=FIRMWARE_NAME;
  json+="\",\"firmwareVersion\":\"";json+=FIRMWARE_VERSION;
  json+="\",\"controller\":\"";
#if CONFIG_IDF_TARGET_ESP32S3
  json+="ESP32-S3";
#elif CONFIG_IDF_TARGET_ESP32C3
  json+="ESP32-C3";
#else
  json+="ESP32";
#endif
  json+="\",\"touchGpio\":";json+=String(TOUCH_PIN);
  json+=",\"pwmGpio\":";json+=String(LED_PWM_PIN);
  json+=",\"uptime\":";
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

String escapeJson(const String &input);

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
  json += "\",\"homeWebAddress\":\"http://";
  json += hostName;
  json += ".local";
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
  json += ",\"mqttHost\":\"";
  json += escapeJson(optionalMqttBrokerHost());
  json += "\"";
  json += ",\"wifi\":";
  json += WiFi.status()==WL_CONNECTED ? "true" : "false";
  json += ",\"rssi\":";
  json += WiFi.status()==WL_CONNECTED ? String(WiFi.RSSI()) : "0";
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
  if(requestFromSetupAP())beginSetupSession();

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
  if(requestFromSetupAP())beginSetupSession();
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
.telemetry{display:flex;flex-wrap:wrap;gap:8px;margin:15px 0}.telemetry{align-items:stretch}.telemetry .setupTile{flex:1 1 210px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;min-width:0;padding:11px;background:#101c2c;border:1px solid #4b6480;border-radius:9px;text-align:center;text-decoration:none;color:#edf5ff;font-size:13px;cursor:pointer}.telemetry .setupTile:hover,.telemetry .setupTile:focus-visible{background:#203b59;border-color:#76b9f2}.telemetry .setupTile:active{background:#235d97}.setupTile .tileHint{font-size:11px;color:#94c9f5}.frontSignal{display:inline-flex;align-items:center;justify-content:center;gap:7px;white-space:nowrap;font-size:12px}.frontSignal .signalBars{flex:none}.frontSignal #frontRssiText{display:inline-block;width:23ch;text-align:left;font-variant-numeric:tabular-nums}.frontSignal .signalBars{display:inline-flex;align-items:flex-end;gap:2px;height:18px}.frontSignal .signalBars i{display:block;width:4px;border-radius:2px 2px 0 0;background:#435367}.frontSignal .signalBars i:nth-child(1){height:5px}.frontSignal .signalBars i:nth-child(2){height:8px}.frontSignal .signalBars i:nth-child(3){height:11px}.frontSignal .signalBars i:nth-child(4){height:14px}.frontSignal .signalBars i:nth-child(5){height:18px}.frontSignal .signalBars i.active{background:currentColor;box-shadow:0 0 5px currentColor}.frontSignal.excellent{color:#4ade80}.frontSignal.good{color:#60a5fa}.frontSignal.marginal{color:#facc15}.frontSignal.weak{color:#fb7185}.frontSignal.offline{color:#94a3b8}
details{text-align:left;margin-top:20px;border-top:2px solid #537492;padding-top:15px}summary{cursor:pointer;color:#add9ff;font-weight:700;letter-spacing:.1em;min-height:44px;padding:8px 0}.phonePanel{margin-top:16px;padding-top:16px;border-top:1px solid #44627e}.phonePanel label{display:block;margin-bottom:9px;font-weight:650}.phonePanel button{margin-left:0}.phonePanel small{display:block;line-height:1.5}.phonePanel p{min-height:1.2em;font-size:14px}
.row{flex-wrap:wrap;overflow-wrap:anywhere}.row span:last-child{max-width:65%}.row span:first-child{flex:1}
@media(max-width:390px){body{padding:0 10px}.card{padding:16px;border-left-width:6px}.row span:last-child{max-width:60%}}
</style></head><body>
<h2 id="wifi-setup">Tech 49 Lab — Wi-Fi setup</h2><p><small>Settings are available on home Wi-Fi or Direct Wi-Fi. Changing networks will restart the lamp and may disconnect this browser.</small></p>
<p>Select your home's Wi-Fi network from the list, then enter its password.</p>
<label>Nearby 2.4 GHz Wi-Fi networks
<select id="networks" onchange="chooseNetwork()" style="display:block;width:100%;font-size:17px;padding:12px;margin:14px 0;border-radius:8px;">
<option value="">Scanning nearby networks...</option></select></label>
<button type="button" onclick="refreshNetworks()" style="background:#2266a3;">Refresh network list</button>
<p id="scanStatus" aria-live="polite">Checking nearby networks...</p>
<form method="post" action="/setup" onsubmit="return confirmSave(event)"><input type="hidden" name="approval" id="approval">
<label>Selected network name (SSID)
<input id="ssid" name="ssid" maxlength="32" placeholder="Select above or type name" required></label>
<label>Home Wi-Fi password
<input name="password" type="password" maxlength="63" autocomplete="new-password"></label>
<p id="approvalMessage" aria-live="polite"></p><button type="submit">Save &amp; connect</button></form>
<p><small>The ESP32-C3 supports 2.4 GHz Wi-Fi. If your network is hidden or
missing from the list, enter the exact network name manually.
The lamp restarts after saving. The touch pads work without Wi-Fi.</small></p>
<p><a href="/">Back to light controls</a></p>
<script>
// Wi-Fi and MQTT are configured separately. Existing MQTT credentials are never erased by Wi-Fi changes.
// Wi-Fi changes require physical touch confirmation.

async function confirmSave(event){
 event.preventDefault();
 const msg=document.getElementById('approvalMessage');
 const form=event.target;
 if(form.dataset.approved==='yes'){form.submit();return false;}
 msg.textContent='Hold the lamp touch pad for 4 seconds to authorize changing Wi-Fi settings.';
 try{
  const r=await fetch('/api/diag/reveal/start',{method:'POST'});
  if(!r.ok)throw Error('Could not start confirmation');
  const {token}=await r.json();
  for(let i=0;i<35;i++){
   await new Promise(resolve=>setTimeout(resolve,500));
   const q=await fetch('/api/diag/reveal/status',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({token})});
   const v=await q.json();
   if(v.state==='confirmed'){
    document.getElementById('approval').value=token;
    form.dataset.approved='yes';form.submit();return false;
   }
   if(v.state==='invalid')throw Error('Confirmation expired');
  }
  throw Error('Timed out waiting for touch confirmation');
 }catch(e){msg.textContent=e.message+' — please try again.';}
 return false;
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
fetch('/api/wifi/config',{cache:'no-store'}).then(r=>r.json()).then(s=>{
 if(s.ssid){document.getElementById('ssid').value=s.ssid;
 document.getElementById('scanStatus').textContent='Current home network: '+s.ssid+'. Leave the password blank to keep the saved password.';}
}).catch(()=>{});
refreshNetworks();
</script></body></html>)html";
  server.send(200,"text/html; charset=utf-8",html);
}

bool approveSettingsChange(){
  // A physical 4-second touch is required for credential changes over either network.
  if(!diagConsumeReveal(server.arg("approval"))){
    server.send(403,"text/plain","Physical touch confirmation required. Return to setup and hold the touch pad for 4 seconds.");
    return false;
  }
  return true;
}

void saveSetup()
{
  if(!approveSettingsChange())return;
  const String ssid=server.arg("ssid");
  const String password=server.arg("password");
  if(ssid.length()<1 || ssid.length()>32 ||
     (password.length()>0 && (password.length()<8 || password.length()>63))) {
    server.send(400,"text/plain","SSID or WPA2 password length invalid");return;
  }
  Preferences prefs;
  if(!prefs.begin("tech49light",false)){
    server.send(500,"text/plain","Cannot open NVS preferences");return;
  }
  const String oldSsid=prefs.getString("ssid","");
  if(password.isEmpty() && ssid!=oldSsid){
    prefs.end();server.send(400,"text/plain","Enter the Wi-Fi password for a new network.");return;
  }
  prefs.putString("ssid",ssid);
  if(!password.isEmpty())prefs.putString("password",password);
  prefs.end();
  server.send(200,"text/html; charset=utf-8",
    "<html><meta name='viewport' content='width=device-width'>"
    "<h2>Wi-Fi settings saved. The light is restarting.</h2>"
    "<p>Reconnect your phone to your home Wi-Fi and open the light's address.</p></html>");
  rebootAt=millis()+1800;
}

void sendMqttSetup(){
  server.sendHeader("Cache-Control","no-store");
  String page=R"mqtt(<!doctype html><html lang="en"><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>Tech 49 Lab MQTT Settings</title>
<style>body{background:#080e18;color:#edf5ff;max-width:520px;margin:25px auto;padding:0 18px;font:17px Arial,sans-serif}h1{color:#83c8ff}.card{background:#182638;border:1px solid #435e7c;border-left:8px solid #348cdd;border-radius:16px;padding:22px}label{display:block;margin:15px 0}input:not([type=checkbox]){display:block;box-sizing:border-box;width:100%;padding:12px;margin-top:6px;border-radius:8px;background:#0c1624;color:white;border:1px solid #6287ad;font-size:17px}button,.nav{display:inline-block;background:#235d97;color:white;border:1px solid #568ac0;border-radius:9px;padding:13px 18px;font-size:16px;text-decoration:none;cursor:pointer}small{color:#b5c7dc}#message{color:#facc15}</style></head><body><div class="card"><small>TECH 49 LAB</small><h1>Home Assistant / MQTT</h1><p>Configure your optional Home Assistant connection. You can access this page from home Wi-Fi or Direct Wi-Fi.</p>
<p id="status">Checking MQTT status...</p><form id="mqttForm" method="post" action="/mqtt" onsubmit="return authorize(event)"><input type="hidden" name="approval" id="approval">
<label><input type="checkbox" name="mqtt" id="enabled" onchange="toggle()"> Enable Home Assistant (MQTT)</label><div id="details">
<label>MQTT broker IP address or hostname<input name="mqttHost" id="host" maxlength="100" placeholder="e.g. 192.168.1.218"></label>
<label>Broker port<input name="mqttPort" id="port" type="number" min="1" max="65535" value="1883"></label>
<label>Username (optional)<input name="mqttUser" id="user" maxlength="64" autocomplete="off"></label>
<label>Password (leave blank to keep existing password)<input name="mqttPass" id="pass" type="password" maxlength="128" autocomplete="new-password"></label></div>
<p><small>For security, saving settings requires holding the lamp's touch pad for four seconds. Your existing password is never displayed.</small></p>
<p id="message" aria-live="polite"></p><button type="submit">Save MQTT Settings</button></form><p><a class="nav" href="/diagnostics">Back to Engineering</a></p></div>
<script>function toggle(){document.getElementById('details').hidden=!document.getElementById('enabled').checked;}
fetch('/api/mqtt/config',{cache:'no-store'}).then(r=>r.json()).then(s=>{
 document.getElementById('enabled').checked=s.enabled;document.getElementById('host').value=s.host||'';document.getElementById('port').value=s.port||1883;document.getElementById('user').value=s.user||'';
 document.getElementById('status').textContent='Status: '+(s.enabled?(s.connected?'Connected':'Disconnected'):'Disabled');toggle();
}).catch(()=>{document.getElementById('status').textContent='Unable to load current settings';toggle();});
async function authorize(event){event.preventDefault();const form=event.target,msg=document.getElementById('message');
 if(form.dataset.approved==='yes'){form.submit();return false;}
 msg.textContent='Hold the lamp touch pad for 4 seconds to confirm MQTT changes.';
 try{const r=await fetch('/api/diag/reveal/start',{method:'POST'});if(!r.ok)throw Error('Confirmation unavailable');const {token}=await r.json();
 for(let i=0;i<35;i++){await new Promise(resolve=>setTimeout(resolve,500));const q=await fetch('/api/diag/reveal/status',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({token})});const v=await q.json();
 if(v.state==='confirmed'){document.getElementById('approval').value=token;form.dataset.approved='yes';form.submit();return false;}
 if(v.state==='invalid')throw Error('Confirmation expired');}throw Error('Touch confirmation timed out');
 }catch(e){msg.textContent=e.message+' — please try again.';}return false;}
</script></body></html>)mqtt";
  server.send(200,"text/html; charset=utf-8",page);
}

void sendMqttConfig(){
  Preferences prefs;
  if(!prefs.begin("tech49mqtt",true)){server.send(500,"application/json","{}");return;}
  String result="{\"enabled\":";
  result+=prefs.getBool("enabled",false)?"true":"false";
  result+=",\"connected\":";result+=optionalMqttConnected()?"true":"false";
  result+=",\"host\":\"";result+=escapeJson(prefs.getString("host",""));
  result+= "\",\"port\":";result+=String(prefs.getUShort("port",1883));
  result+=",\"user\":\"";result+=escapeJson(prefs.getString("user",""));result+="\"}";
  prefs.end();server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",result);
}

void saveMqttSetup(){
  if(!approveSettingsChange())return;
  const bool enabled=server.hasArg("mqtt");
  String host=server.arg("mqttHost"),user=server.arg("mqttUser"),pass=server.arg("mqttPass");
  String portText=server.arg("mqttPort");
  if(enabled){
    if(host.isEmpty()||host.length()>100||user.length()>64||pass.length()>128||portText.isEmpty()||portText.length()>5){server.send(400,"text/plain","Invalid MQTT settings");return;}
    for(size_t i=0;i<portText.length();i++)if(portText[i]<'0'||portText[i]>'9'){server.send(400,"text/plain","Invalid port");return;}
  }
  unsigned long port=portText.toInt();if(enabled&&(port<1||port>65535)){server.send(400,"text/plain","Invalid port");return;}
  if(enabled&&pass.isEmpty()){
    Preferences prefs;if(prefs.begin("tech49mqtt",true)){pass=prefs.getString("pass","");prefs.end();}
  }
  if(!optionalMqttSaveSettings(enabled,host,enabled?static_cast<uint16_t>(port):1883,user,pass)){
    server.send(500,"text/plain","Unable to save MQTT settings");return;
  }
  server.send(200,"text/html; charset=utf-8","<html><meta name='viewport' content='width=device-width'><h2>MQTT settings saved.</h2><p>The lamp will restart to apply MQTT settings. Wi-Fi credentials are unchanged.</p><a href='/diagnostics'>Engineering</a></html>");
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
  char suffix[13];
  touchLightDeviceId(ESP.getEfuseMac(), suffix);
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

  Serial.println("Wi-Fi setup: Universal V1.3.0 Console (test).");
  diagLog("[SYSTEM] Firmware V1.3.0 started");
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
  server.on("/mqtt",HTTP_GET,sendMqttSetup);
  server.on("/api/wifi/config",HTTP_GET,[](){
    String result="{\"ssid\":\""+escapeJson(savedSSID)+"\"}";
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json",result);
  });
  server.on("/mqtt",HTTP_POST,saveMqttSetup);
  server.on("/api/mqtt/config",HTTP_GET,sendMqttConfig);
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

