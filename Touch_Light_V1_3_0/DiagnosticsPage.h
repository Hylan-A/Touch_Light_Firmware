#pragma once
#include <Arduino.h>
// Served only from the ESP32-C3 local web server; no external assets or cloud.
static const char DIAGNOSTICS_PAGE[] PROGMEM = R"diaghtml(
<!doctype html><html lang="en"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Tech 49 Lab Diagnostics</title>
<style>
:root{color-scheme:dark}
body{background:#0e1725;color:#f3f7ff;font:16px -apple-system,Arial,sans-serif;
max-width:660px;margin:20px auto;padding:0 15px}
h1{font-size:24px;color:#7dd3fc}
.card{background:#1b2b40;padding:16px;margin:12px 0;border-radius:15px}
.row{display:flex;justify-content:space-between;gap:10px;
padding:5px 0;border-bottom:1px solid #40526a}
.row span:last-child{font-weight:600;text-align:right}
button{font:16px -apple-system,Arial,sans-serif;color:white;
background:#087b4a;padding:12px;border:0;border-radius:9px;margin:7px 0}
#log{background:#070f19;color:#c6f6d5;white-space:pre-wrap;word-break:break-word;
font:12px ui-monospace,Menlo,monospace;min-height:200px;padding:12px;border-radius:8px}
a{color:#7dd3fc}p{line-height:1.5}small{color:#b8cce3}
#secret{white-space:pre-wrap;word-break:break-word;color:#ffd38a;
font:15px ui-monospace,Menlo,monospace}
/* Readable Wi-Fi signal rating with a dot; color is not the only cue. */
.signalQuality{display:inline-flex;align-items:center;justify-content:flex-end;gap:7px;min-width:0}.signalQuality #rssiText{display:inline-block;width:23ch;text-align:left;font-variant-numeric:tabular-nums}
.signalBars{display:inline-flex;align-items:flex-end;gap:3px;height:22px;flex:none}
.signalBars i{display:block;width:5px;border-radius:2px 2px 0 0;background:#435367;border:1px solid #70809944}
.signalBars i:nth-child(1){height:6px}.signalBars i:nth-child(2){height:10px}
.signalBars i:nth-child(3){height:14px}.signalBars i:nth-child(4){height:18px}
.signalBars i:nth-child(5){height:22px}
.signalBars i.active{background:currentColor;border-color:currentColor;box-shadow:0 0 5px currentColor}
.signalQuality.excellent{color:#4ade80}
.signalQuality.good{color:#60a5fa}
.signalQuality.marginal{color:#facc15}
.signalQuality.weak{color:#fb7185}
.signalQuality.offline{color:#94a3b8}
.wifiGuide{display:block;margin-top:12px;line-height:1.5}
.setupAction{display:inline-flex;align-items:center;justify-content:center;background:#235d97;border:1px solid #61b9ff;color:#fff;text-decoration:none;border-radius:9px;min-height:40px;padding:7px 13px;font-weight:650;font-size:14px}.setupAction:hover,.setupAction:focus-visible{background:#3479b6}
/* Enlarged iPhone nav controls: an X closes diagnostics, not the iOS app. */
.diagNav{display:flex;justify-content:space-between;align-items:center;
gap:12px;margin:0 0 22px}
.diagNavButton{display:inline-flex;align-items:center;justify-content:center;
min-height:50px;padding:12px 16px;box-sizing:border-box;border:0;
border-radius:11px;background:#2563a6;color:white;
font:600 16px -apple-system,BlinkMacSystemFont,Arial,sans-serif;
cursor:pointer;touch-action:manipulation;
-webkit-tap-highlight-color:transparent}
.diagNavButton:active{background:#1a4d88}
.diagNavButton:focus-visible{outline:3px solid #7dd3fc;outline-offset:3px}
.brightnessGuide{display:block;margin-top:12px;line-height:1.5}
/* Button colors show current action: red Pause when live; green Resume when paused. */
.logTools{display:flex;flex-wrap:wrap;align-items:stretch;gap:8px;margin:14px 0 8px}
.logTools button{flex:1 1 135px;min-height:52px;margin:0;
font-size:16px;font-weight:700;touch-action:manipulation}
.logTools button.pause{background:#b91c1c}
.logTools button.resume{background:#15803d}
.logTools button.clear{background:#475569}
.logTools button.save{background:#2563eb}
.logTools button:active{filter:brightness(1.2)}
#logStatus{display:block;margin-top:10px;padding:9px 11px;border-radius:9px;
font-size:14px;font-weight:600;text-align:left}
#logStatus.live{color:#b8f7d4;background:#104e32}
#logStatus.paused{color:#fecaca;background:#642121}
#logAction{min-height:1.2em;display:block;margin:5px 0;font-size:13px;color:#b8cce3}
/* Reboot is intentionally separate from normal lamp/log controls. */
#restartButton{background:#b45309;min-height:52px;font-weight:700}
#restartButton:disabled{background:#64748b;opacity:.75}
#restartMessage{color:#fde68a;min-height:1.3em}
.soundSetting{display:flex;align-items:center;gap:14px;flex-wrap:wrap}
.soundButton{background:linear-gradient(160deg,#338ac9,#16456e);border:1px solid #8bd2ff;box-shadow:inset 0 2px 2px #fff5,0 3px 8px #0008;min-width:190px}
.soundButton[aria-pressed=false]{background:linear-gradient(160deg,#39506a,#1a2939);border-color:#66839b}
#buttonSoundState{min-width:40px;color:#a8edce}
.holdButton{position:relative;overflow:hidden;user-select:none;-webkit-user-select:none;touch-action:none;cursor:pointer}
.holdButton:before{content:"";position:absolute;inset:0 auto 0 0;width:var(--hold-progress,0%);background:#ffffff38;pointer-events:none}
.holdButton.holding{outline:2px solid #fcd34d}
.holdButton:disabled{cursor:default}
.resetHint{display:block;line-height:1.5;margin-top:12px}

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


#log{color:#e7effa;white-space:normal;padding:8px;overflow-wrap:anywhere}
.logEvent{white-space:pre-wrap;border-left:5px solid;padding:8px 10px;margin:5px 0;font:12px ui-monospace,Menlo,monospace;border-radius:3px;break-inside:avoid}
.event-on{background:#48232e;color:#ffc9d1;border-color:#f2667e}.event-off{background:#173e31;color:#b5f3cc;border-color:#48cc88}.event-system{background:#40371c;color:#ffe69a;border-color:#e2ba46}.event-other{background:#342b45;color:#e2cdfb;border-color:#b493da}
.legend{display:flex;flex-wrap:wrap;gap:8px}.legend span{padding:6px 10px;border-left:4px solid;font-size:13px}

.led{display:inline-block;width:13px;height:13px;border-radius:50%;background:#687588;margin:0 8px;vertical-align:middle}.led.green{background:#39d98a;box-shadow:0 0 7px #39d98a}.led.red{background:#f05266}.blink{animation:ledBlink 1s steps(2,end) infinite}@keyframes ledBlink{50%{opacity:.2}}.led.watchdogOff{background:#18202b;box-shadow:none}
</style></head><body>
<nav class="diagNav" aria-label="Diagnostics navigation">
<button type="button" class="diagNavButton"
onclick="window.location.assign('/')"
aria-label="Back to Touch Light controls">&#8592; Back to Lamp</button>
</nav>
<h1>TECH 49 LAB — Engineering</h1>
<div class="card">
<div class="row"><span>Device</span><span id="device">—</span></div>
<div class="row"><span>Firmware</span><span id="version">—</span></div>
<div class="row"><span>Board ID</span><span id="boardId">—</span></div>
<div class="row"><span>Hardware revision</span><span id="hardwareRev">—</span></div>
<div class="row"><span>Firmware type</span><span id="firmwareType">—</span></div>
<div class="row"><span>Firmware version</span><span id="firmwareVersion">—</span></div>
<div class="row"><span>Controller</span><span id="controller">—</span></div>
<div class="row"><span>Touch GPIO</span><span id="touchGpio">—</span></div>
<div class="row"><span>PWM GPIO</span><span id="pwmGpio">—</span></div>
<div class="row"><span>Local time (browser)</span><span id="clock">—</span></div>
<div class="row"><span>Uptime</span><span id="uptime">—</span></div>
<div class="row"><span>Home Wi-Fi</span><span id="wifi">—</span></div>
<div class="row"><span>Home Wi-Fi Setup</span><span><a class="setupAction" href="/setup#wifi-setup">⚙ Configure Wi-Fi</a></span></div>
<div class="row"><span>Signal strength</span>
<span id="rssi" class="signalQuality offline">
<span class="signalBars" aria-hidden="true"><i></i><i></i><i></i><i></i><i></i></span>
<span id="rssiText">Checking...</span>
</span></div>
<div class="row"><span>IP address</span><span id="ip">—</span></div>
<div class="row"><span>Direct Wi-Fi</span><span id="ap">—</span></div>
<div class="row"><span>Home Assistant MQTT</span><span id="mqtt">—</span></div>
<div class="row"><span>Home Assistant MQTT Setup</span><span><a class="setupAction" href="/mqtt">⚙ Configure MQTT</a></span></div>
<small class="wifiGuide">Wi-Fi signal guide: Excellent (−60 dBm or stronger),
Good (−61 to −70 dBm), Marginal (−71 to −80 dBm),
Weak (below −80 dBm). Gray means not connected.</small>
</div>
<div class="card"><h2>Browser preferences</h2>
<div class="soundSetting"><button type="button" id="buttonSoundToggle" class="soundButton" aria-pressed="true" onclick="setButtonSound(!buttonSound)" aria-label="Toggle button sound">🔊 Button Sound</button><strong id="buttonSoundState" role="status">ON</strong></div>
<p><small>Same short beep for every web button. Saved separately on this browser; the physical touch pad remains silent.</small></p>
<div class="phonePanel"><label for="phoneName">Name this phone or browser</label>
<input id="phoneName" maxlength="48" placeholder="e.g. Hylan’s iPhone" autocomplete="off">
<button type="button" onclick="savePhoneName()">Save name</button>
<p id="phoneMessage" role="status" aria-live="polite"></p>
<small>Saved on this browser for this lamp address. Used to identify web commands in the diagnostic log. Home Assistant retains its own label.</small></div>

</div>
<div class="card"><h2>Live system log</h2>
<small>Updates every two seconds. These messages live in RAM and reset
when the lamp restarts. Events are shown in your browser's local time
(including daylight saving time); no internet or router clock is needed.
The browser device's clock must be set correctly. Passwords are not included.</small>
<small class="brightnessGuide">Brightness is shown as
<strong>8-bit PWM counts (0–255)</strong> and
<strong>percent PWM duty cycle</strong>, not measured LED light output.
When the lamp is OFF, the log shows the saved brightness preset and
the actual PWM output is 0%.</small>
<p class="legend"><span class="event-on">ON — red</span><span class="event-off">OFF — green</span><span class="event-system">Diagnostics — amber</span><span class="event-other">Other — lavender</span></p>
<div id="log" role="log" aria-label="Recent diagnostic events">Waiting for data...</div>
<div class="logTools" role="group" aria-label="Live log controls">
<button type="button" id="pauseButton" class="pause"
onclick="toggleLogPause()">&#9208; Pause</button>
<button type="button" id="clearLogButton" class="clear holdButton" data-hold="clear" aria-label="Hold for two seconds to clear log">&#10005; Hold 2 sec — Clear Log</button>
<button type="button" class="save"
onclick="saveLog()">&#8681; Save Text</button>
<button type="button" class="save" onclick="exportColorLog(false)">Save Color Log</button>
<button type="button" class="save" onclick="exportColorLog(true)">Print Color Log</button>
</div>

<small>Pause freezes only this display. The ESP32 continues collecting
up to 36 recent events; the oldest are overwritten automatically.
Clear resets that RAM history without changing lamp, Wi-Fi or MQTT settings.
Save Text keeps the plain text format. Save Color Log makes an HTML file; open it in a browser to view or print. Print Color Log opens a report with a Back to Diagnostics button. If printing does not open in the Home Screen app, open Diagnostics in Safari and use Share → Print. Enable background graphics for colored highlights; ON/OFF labels remain readable in black and white.</small>
<small id="logAction" aria-live="polite"></small>
</div>
<div class="card"><h2>Setup Wi-Fi label</h2>
<p>Recover the lamp's unique Wi-Fi password for its QR-code label.
Tap the button, then <strong>hold the physical touch pad for four seconds</strong>.
During this brief confirmation, normal touch functions are paused.</p>
<button onclick="beginReveal()">Show setup information</button>
<p id="revealMessage">No confirmation requested.</p>
<pre id="secret"></pre>
<small>The password disappears after 45 seconds. Copy it onto a private
label; do not post it publicly.</small></div>
<div class="card" id="recovery">
<h2>Controller recovery &amp; restart</h2>
<div class="row"><span>Watchdog</span><span><i id="watchdogLed" class="led" aria-hidden="true"></i><span id="watchdogState">Checking...</span></span></div>
<div class="row"><span>Last restart cause</span><span id="resetCause">—</span></div>
<div class="row"><span>Last boot (browser estimate)</span><span id="lastBootTime">—</span></div>
<h3>Since test reset</h3><p id="testResetTime">No test reset recorded</p>
<div class="row"><span>Recorded starts</span><span id="resetBoots">—</span></div>
<div class="row"><span>Watchdog resets</span><span id="resetWatchdog">—</span></div>
<div class="row"><span>Web restarts</span><span id="resetWeb">—</span></div>
<button type="button" id="resetCountsButton" class="holdButton" data-hold="reset" aria-label="Hold for two seconds to reset test counts">Hold 2 sec — Reset Test Counts</button><p id="testResetMessage" role="status"></p>
<h3>Lifetime totals</h3>
<div class="row"><span>Recorded starts</span><span id="lifetimeBoots">—</span></div>
<div class="row"><span>Watchdog resets</span><span id="lifetimeWatchdog">—</span></div>
<div class="row"><span>Web restarts</span><span id="lifetimeWeb">—</span></div>
<small class="resetHint">Watchdog recovery runs automatically when the
controller's main loop stops responding for about 60 seconds.
Counts survive restarts. The previous boot time is estimated using this
iPhone's clock; no historical real-time clock is installed.</small>
<button type="button" id="restartButton"
class="holdButton" data-hold="restart" aria-label="Hold for two seconds to restart controller">&#8635; Hold 2 sec — Restart Controller</button>
<p><i id="restartLed" class="led" aria-hidden="true"></i><span id="restartStatus" role="status">Checking connection</span></p>
<p id="restartMessage" role="status" aria-live="polite"></p>
<small>Restart does NOT erase Wi-Fi or Home Assistant settings.
The lamp returns OFF and this page temporarily disconnects.
A web restart works only while the controller is responsive;
a watchdog may recover a software hang. This is NOT factory reset.</small>
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

function updateSoundSetting(){
 const c=document.getElementById('buttonSoundToggle');
 if(c){c.setAttribute('aria-pressed',String(buttonSound));document.getElementById('buttonSoundState').textContent=buttonSound?'ON':'OFF';}
}
function setButtonSound(enabled){
 buttonSound=enabled;
 try{localStorage.setItem(SOUND_KEY,enabled?'on':'off');}
 catch(e){document.getElementById('buttonSoundState').textContent='Storage unavailable';return;}
 updateSoundSetting();
}
document.addEventListener('DOMContentLoaded',updateSoundSetting);

const PHONE_NAME_KEY='tech49.browserName.v1';
let browserName='';
try{browserName=localStorage.getItem(PHONE_NAME_KEY)||'';}catch(e){}
function savePhoneName(){
 const input=$('phoneName'),name=input.value.trim();
 if(new TextEncoder().encode(name).length>48 || /[\x00-\x1f\x7f\[\]]/.test(name)){
  set('phoneMessage','Use up to 48 UTF-8 bytes, without brackets or control characters.');return;
 }
 try{localStorage.setItem(PHONE_NAME_KEY,name);browserName=name;
  set('phoneMessage',name?'✓ Name Saved Successfully — Current Browser: '+name+'. Web commands will use this name in diagnostic logs.':'✓ Name cleared; web commands will show WEB.');
 }catch(e){set('phoneMessage','Browser storage unavailable; name was not saved.');}
}
document.addEventListener('DOMContentLoaded',()=>{
 $('phoneName').value=browserName;
 set('phoneMessage',browserName?'Current Browser: '+browserName:'Current Browser: Unnamed');
});

let token='',shownAt=0;
let revealPolling=false;
let revealGeneration=0;
let logPaused=false;
let latestSnapshot=null;
let latestSnapshotTime=0;
let lastHeard=0,lastFeed=null,lastFeedChange=0,restartPending=false,restartBaseline=null,restartStarted=0;
// Visible 500 ms ON / 500 ms OFF; enabled only by fresh reported feed progress.
function updateWatchdogLed(now){
 const fresh=lastFeedChange>0 && now-lastFeedChange<=7000 && now-lastHeard<=7000;
 $('watchdogLed').className=fresh?(Math.floor((now-lastFeedChange)/500)%2===0?'led green':'led watchdogOff'):'led';
}
setInterval(()=>updateWatchdogLed(Date.now()),50);
function restartStatus(kind,text){$('restartLed').className='led '+kind;set('restartStatus',text);}
async function resetTestCounts(){
 try{await post('/api/diag/test/reset',{epoch:String(Date.now())});set('testResetMessage','Test counts reset.');await refresh();}
 catch(e){set('testResetMessage','Reset not confirmed. Check connection and retry.');}
}
// Destructive operations require a continuous 2-second hold; releasing,
// dragging away, switching windows, or canceling touch aborts the operation.
function installHoldButton(id,action){
 const b=$(id);let timer=null,frame=null,start=0,triggered=false,active=false;
 const cancel=()=>{if(timer!==null)clearTimeout(timer);if(frame!==null)cancelAnimationFrame(frame);
  timer=null;frame=null;active=false;b.classList.remove('holding');b.style.removeProperty('--hold-progress');};
 const tick=()=>{if(!active)return;b.style.setProperty('--hold-progress',Math.min(100,(performance.now()-start)/20)+'%');frame=requestAnimationFrame(tick);};
 b.addEventListener('pointerdown',e=>{
  if(b.disabled || !e.isPrimary || (e.pointerType==='mouse'&&e.button!==0))return;
  e.preventDefault();cancel();active=true;triggered=false;start=performance.now();
  try{b.setPointerCapture(e.pointerId);}catch(err){}
  b.classList.add('holding');tick();
  timer=setTimeout(()=>{if(!active||b.disabled)return;triggered=true;cancel();action();},2000);
 });
 b.addEventListener('pointerup',cancel);b.addEventListener('pointercancel',cancel);
 b.addEventListener('lostpointercapture',cancel);
 b.addEventListener('pointermove',e=>{if(active){const r=b.getBoundingClientRect();
  if(e.clientX<r.left||e.clientX>r.right||e.clientY<r.top||e.clientY>r.bottom)cancel();}});
 window.addEventListener('blur',cancel);document.addEventListener('visibilitychange',()=>{if(document.hidden)cancel();});
 // Keyboard accessibility: holding Space/Enter for two seconds also works.
 b.addEventListener('keydown',e=>{if((e.key===' '||e.key==='Enter')&&!e.repeat&&!active){
  e.preventDefault();active=true;start=performance.now();b.classList.add('holding');tick();
  timer=setTimeout(()=>{if(!active)return;cancel();action();},2000);}});
 b.addEventListener('keyup',e=>{if(e.key===' '||e.key==='Enter'){e.preventDefault();cancel();}});
 b.addEventListener('click',e=>e.preventDefault());
}
document.addEventListener('DOMContentLoaded',()=>{
 installHoldButton('clearLogButton',clearLog);
 installHoldButton('resetCountsButton',resetTestCounts);
 installHoldButton('restartButton',requestRestart);
});
setInterval(()=>{
 const now=Date.now();
 if(now-lastHeard>7000){$('watchdogLed').className='led';set('watchdogState','Updates stale / connection unavailable');if(!restartPending)restartStatus('','Offline');}
 if(restartPending && now-restartStarted>60000){restartStatus('red','Reconnect failed — still retrying');$('restartButton').disabled=false;}
},1000);
const $=id=>document.getElementById(id);
const set=(id,t)=>{$(id).textContent=t;};

function eventClass(message){
 // Classify the event body, never a user-supplied name in the source tag.
 const body=String(message).replace(/^\[[^\]]*\]\s*/, '');
 if(/^Light OFF\b/.test(body))return 'event-off';
 if(/^Light ON\b/.test(body))return 'event-on';
 if(/^\[(SYSTEM|RESET|WATCHDOG|WIFI|MQTT)\]/.test(message))return 'event-system';
 return 'event-other';
}
function renderColorLog(x,now){
 const log=$('log');log.replaceChildren();
 for(const e of x.events||[]){
  const row=document.createElement('div');row.className='logEvent '+eventClass(e.msg);
  row.textContent='['+eventTime(now,x.uptime,e.ms)+'] '+e.msg;log.appendChild(row);
 }
 if(!log.childNodes.length)log.textContent='No events yet.';
}
function escapeHtml(value){return String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}
function colorReport(snapshot,now){
 const fields=[['Device',snapshot.device],['Firmware',snapshot.firmware],['Exported',new Date(now).toLocaleString()],['Home Wi-Fi',snapshot.wifi?'Connected':'Disconnected'],['Signal',snapshot.wifi?snapshot.rssi+' dBm':'Unavailable'],['IP',snapshot.ip],['MQTT',snapshot.mqttEnabled?(snapshot.mqttConnected?'Connected':'Disconnected'):'Disabled'],['Previous reset',snapshot.resetCause||'Unknown'],['Watchdog',snapshot.watchdogActive?'Active (60 seconds)':'Unavailable'],['Test counts reset (browser time)',snapshot.testResetEpoch?new Date(Number(snapshot.testResetEpoch)).toLocaleString():'Never'],['Test starts',snapshot.testBoots],['Test watchdog resets',snapshot.testWatchdog],['Test web restarts',snapshot.testWeb],['Lifetime recorded starts',snapshot.resetBoots],['Watchdog resets',snapshot.resetWatchdog],['Web restarts',snapshot.resetWeb],['Last boot (browser estimate)',Number.isFinite(snapshot.uptimeSec)?new Date(now-snapshot.uptimeSec*1000).toLocaleString():'Unavailable']];
 const rows=(snapshot.events||[]).map(e=>'<div class="logEvent '+eventClass(e.msg)+'">'+escapeHtml('['+eventTime(now,snapshot.uptime,e.ms)+'] '+e.msg)+'</div>').join('');
 return '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Tech 49 Lab Diagnostic Log</title><style>body{max-width:960px;margin:24px auto;padding:0 18px;color:#172332;background:white;font:14px Arial,sans-serif}h1{color:#215b8f;border-bottom:8px solid #7da4c7;padding-bottom:12px}dl{display:grid;grid-template-columns:minmax(120px,1fr) 3fr;gap:7px}dt{font-weight:bold}dd{margin:0;overflow-wrap:anywhere}.logEvent{white-space:pre-wrap;overflow-wrap:anywhere;padding:9px;margin:5px 0;border-left:6px solid;font:12px monospace;break-inside:avoid}.event-on{background:#ffd5dd;border-color:#c72946}.event-off{background:#d1f2df;border-color:#16824b}.event-system{background:#fff0bc;border-color:#c39a21}.event-other{background:#eee1fa;border-color:#9963b8}@media print{body{margin:0;max-width:none}*{-webkit-print-color-adjust:exact;print-color-adjust:exact}.printButton{display:none}}</style></head><body><h1>TECH 49 LAB — DIAGNOSTIC LOG</h1><dl>'+fields.map(([k,v])=>'<dt>'+escapeHtml(k)+'</dt><dd>'+escapeHtml(v)+'</dd>').join('')+'</dl><p>ON: red · OFF: green · Diagnostics: amber · Other: lavender</p><p>Most recent '+(snapshot.events||[]).length+' events; volatile RAM history. Brightness values are PWM counts /255 and duty cycle, not measured light output. Event times estimated from browser clock.</p>'+rows+'<p>For color highlights, enable background graphics in your print settings.</p><button class="printButton" onclick="window.print()">Print / Save as PDF</button></body></html>';
}
function exportColorLog(printRequested){
 if(!latestSnapshot){set('logAction','Waiting for diagnostic data.');return;}
 const content=colorReport(latestSnapshot,latestSnapshotTime);
 if(printRequested){
  const panel=document.createElement('div');panel.id='reportPanel';
  panel.style='position:fixed;inset:0;z-index:1000;background:white;display:flex;flex-direction:column';
  const bar=document.createElement('div');bar.style='padding:12px;background:#172332;color:white';
  const back=document.createElement('button');back.textContent='Back to Diagnostics';back.onclick=()=>panel.remove();bar.appendChild(back);
  const print=document.createElement('button');print.textContent='Print / Save as PDF';bar.appendChild(print);
  const hint=document.createElement('p');hint.textContent='If printing does not open from the Home Screen app, open this diagnostics page in Safari and use Share → Print.';bar.appendChild(hint);
  const frame=document.createElement('iframe');frame.title='Color diagnostic report';frame.style='flex:1;width:100%;border:0';frame.srcdoc=content;
  print.onclick=()=>{try{frame.contentWindow.focus();frame.contentWindow.print();}catch(e){hint.textContent='Open Diagnostics in Safari, then use Share → Print.';}};
  panel.append(bar,frame);document.body.appendChild(panel);return;
 }
 const blob=new Blob([content],{type:'text/html;charset=utf-8'});
 const url=URL.createObjectURL(blob),link=document.createElement('a');
 link.href=url;link.download='Tech49_'+String(latestSnapshot.device||'TouchLight').replace(/[^a-zA-Z0-9_-]/g,'_')+'_Diagnostics.html';
 document.body.appendChild(link);link.click();link.remove();setTimeout(()=>URL.revokeObjectURL(url),30000);
 set('logAction',printRequested?'Popup blocked; color report download requested.':'Color report download requested.');
}

function renderLog(x,now){
 return (x.events||[]).map(e=>
  '['+eventTime(now,x.uptime,e.ms)+'] '+e.msg
 ).join('\n')||'No events yet.';
}
function updatePauseButton(){
 const button=$('pauseButton');
 if(logPaused){
  button.className='resume';
  button.textContent='▶ Resume';
 }else{
  button.className='pause';
  button.textContent='⏸ Pause';
 }
}
function toggleLogPause(){
 logPaused=!logPaused;
 updatePauseButton();
 if(!logPaused)refresh();
}
async function clearLog(){
 try{
  await post('/api/diag/clear');
  logPaused=false;updatePauseButton();
  set('logAction','RAM log cleared — new events will appear as they occur.');
  await refresh();
 }catch(e){set('logAction','Could not clear log. Check Wi-Fi and try again.');}
}
function saveLog(){
 if(!latestSnapshot){
  set('logAction','Waiting for diagnostic data before saving.');
  return;
 }
 // Use the latest server snapshot, including events received while
 // this browser view is paused. No setup password is ever exported.
 const snapshot=latestSnapshot;
 const now=latestSnapshotTime;
 const lines=[
  'TECH 49 LAB — TOUCH LIGHT DIAGNOSTIC LOG',
  'Device: '+snapshot.device,
  'Firmware: '+snapshot.firmware,
  'Exported (phone local time): '+new Date(now).toLocaleString(),
  'Home Wi-Fi: '+(snapshot.wifi?'Connected':'Disconnected'),
  'Signal strength: '+(snapshot.wifi?snapshot.rssi+' dBm':'Not available'),
  'IP address: '+snapshot.ip,
  'MQTT: '+(snapshot.mqttEnabled?(snapshot.mqttConnected?'Connected':'Disconnected'):'Disabled'),
  'Previous reset: '+(snapshot.resetCause||'Unknown'),
  'Watchdog: '+(snapshot.watchdogActive?'Active (60 seconds)':'Unavailable'),
  'Recorded boots: '+snapshot.resetBoots,
  'Watchdog resets: '+snapshot.resetWatchdog,
  'Web restarts: '+snapshot.resetWeb,
  'Last boot (phone estimate): '+(Number.isFinite(snapshot.uptimeSec)?
   new Date(now-snapshot.uptimeSec*1000).toLocaleString():'Unavailable'),
  'Events: '+(snapshot.events||[]).length+' (most recent; volatile RAM buffer)',
  'Brightness values are PWM counts /255 and percentage duty cycle.',
  '------------------------------------------------------------',
  renderLog(snapshot,now),
  ''
 ];
 const content=lines.join('\n');
 const safeId=(snapshot.device||'TouchLight').replace(/[^a-zA-Z0-9_-]/g,'_');
 const filename='Tech49_'+safeId+'_Diagnostics.txt';
 const blob=new Blob([content],{type:'text/plain;charset=utf-8'});
 const url=URL.createObjectURL(blob);
 const link=document.createElement('a');
 link.href=url;link.download=filename;
 document.body.appendChild(link);link.click();link.remove();
 setTimeout(()=>URL.revokeObjectURL(url),30000);
 set('logAction','Save requested: '+filename+' — check Safari Downloads or Files.');
}
function duration(ms){
 let t=Math.floor(ms/1000);
 return Math.floor(t/3600)+'h '+Math.floor(t%3600/60)+'m '+(t%60)+'s';
}
function localClock(now){
 return new Date(now).toLocaleString(undefined,{
  month:'short',day:'numeric',hour:'numeric',minute:'2-digit',
  second:'2-digit',hour12:true
 });
}
function classifySignal(rssi,connected){
 if(!connected || !Number.isFinite(rssi)){
  return {rating:'Not connected',css:'offline'};
 }
 if(rssi>=-60)return {rating:'Excellent',css:'excellent'};
 if(rssi>=-70)return {rating:'Good',css:'good'};
 if(rssi>=-80)return {rating:'Marginal',css:'marginal'};
 return {rating:'Weak',css:'weak'};
}
function updateSignal(rssi,connected){
 const state=classifySignal(rssi,connected);
 const indicator=$('rssi');
 indicator.className='signalQuality '+state.css;
 const bars=indicator.querySelectorAll('.signalBars i');
 const count=!connected?0:rssi>=-60?5:rssi>=-67?4:rssi>=-73?3:rssi>=-80?2:1;
 bars.forEach((bar,index)=>bar.classList.toggle('active',index<count));
 set('rssiText',state.rating+(connected?' ('+rssi+' dBm)':''));
 indicator.setAttribute('aria-label','Wi-Fi signal: '+state.rating+
  (connected?', '+rssi+' dBm':''));
}
function eventTime(phoneNow, boardUptime, eventUptime){
 // Both values are milliseconds since the ESP32 started. The unsigned
 // difference also handles millis() wrapping after about 49.7 days.
 const elapsedMs=(boardUptime-eventUptime)>>>0;
 const eventDate=new Date(phoneNow-elapsedMs);
 const sameDay=eventDate.toDateString()===new Date(phoneNow).toDateString();
 const time=eventDate.toLocaleTimeString(undefined,{
  hour:'numeric',minute:'2-digit',second:'2-digit',hour12:true
 });
return eventDate.toLocaleDateString()+' '+time;
}
async function refresh(){
 try{
 const res=await fetch('/api/diagnostics',{cache:'no-store'});
 if(!res.ok)throw Error('no connection');
 const x=await res.json();
 const phoneNow=Date.now(); // Local wall clock; available even without internet.
 set('device',x.device);set('version',x.firmware);
  for(const [id,key] of [['boardId','boardId'],['hardwareRev','hardwareRev'],['firmwareType','firmwareType'],['firmwareVersion','firmwareVersion'],['controller','controller'],['touchGpio','touchGpio'],['pwmGpio','pwmGpio']])set(id,x[key]??'—');
 set('clock',localClock(phoneNow));
 set('uptime',duration(x.uptime));
 set('wifi',x.wifi?'Connected':'Not connected');
 updateSignal(x.rssi,x.wifi);
 set('ip',x.ip);set('ap',x.ap?'Active':'Inactive');
 set('mqtt',x.mqttEnabled?(x.mqttConnected?'Connected':'Enabled, disconnected'):'Disabled');
 set('watchdogState',x.watchdogActive?'Active (60 seconds)':'Unavailable / not armed');
 set('resetCause',x.resetCause||'Unknown');
 set('resetBoots',String(x.testBoots??'—'));
 set('lifetimeBoots',String(x.resetBoots??'—'));
 set('lifetimeWatchdog',String(x.resetWatchdog??'—'));
 set('lifetimeWeb',String(x.resetWeb??'—'));
 set('testResetTime',x.testResetEpoch?'Test counts reset on '+new Date(Number(x.testResetEpoch)).toLocaleString()+' (browser time)':'No test reset recorded; counts include all recorded starts');
 set('resetWatchdog',String(x.testWatchdog??'—'));
 set('resetWeb',String(x.testWeb??'—'));
 set('lastBootTime',Number.isFinite(x.uptimeSec)?
  new Date(phoneNow-x.uptimeSec*1000).toLocaleString():'Unavailable');
 lastHeard=phoneNow;
 if(x.watchdogActive && lastFeed!==null && x.watchdogFeeds!==lastFeed){
  lastFeedChange=phoneNow;
 }else if(!x.watchdogActive){lastFeedChange=0;}
 updateWatchdogLed(phoneNow);
 lastFeed=x.watchdogFeeds;
 if(restartPending){
  if(restartBaseline!==null && x.resetBoots>restartBaseline){restartPending=false;restartStatus('green','Online — new boot confirmed');$('restartButton').disabled=false;set('restartMessage','Restart complete. Controller reconnected.');}
 }else restartStatus('green','Online');
 latestSnapshot=x;latestSnapshotTime=phoneNow;
 if(!logPaused)renderColorLog(x,phoneNow);
 }catch(e){set('wifi','Connection lost');}
}
async function requestRestart(){
 // A confirmed request only: leave factory reset on the physical touch pad.
 if(!latestSnapshot){set('restartMessage','Wait for a controller response before restarting.');return;}
 restartBaseline=latestSnapshot.resetBoots;
 const button=$('restartButton');
 button.disabled=true;
 set('restartMessage','Sending restart request...');
 try{
  await post('/api/diag/restart');
  restartPending=true;restartStarted=Date.now();restartStatus('red blink','Restarting');
  set('restartMessage','Restart accepted. Lamp turns OFF. Waiting for a confirmed new boot...');
 }catch(e){
  restartStatus('','Restart not confirmed');
  set('restartMessage','Restart was not confirmed; check connection and retry.');
  button.disabled=false;
 }
}
async function post(path,args){
 const r=await fetch(path,{method:'POST',
 headers:{'Content-Type':'application/x-www-form-urlencoded'},
 body:new URLSearchParams(args||{})});
 if(!r.ok)throw Error('HTTP '+r.status);
 return r.json();
}
async function beginReveal(){
  // Invalidate any older status/consume request before starting a new challenge.
  const generation=++revealGeneration;
  token='';shownAt=0;$('secret').textContent='';
  set('revealMessage','Starting physical confirmation...');
  try{
    const result=await post('/api/diag/reveal/start');
    if(generation!==revealGeneration)return;
    token=result.token;
    set('revealMessage','Hold the PHYSICAL touch pad for four seconds...');
  }catch(e){
    if(generation===revealGeneration)set('revealMessage','Unable to start confirmation.');
  }
}
async function pollReveal(){
  if(shownAt && Date.now()-shownAt>45000){
    $('secret').textContent='';shownAt=0;
    set('revealMessage','Password hidden again.');
  }
  // Prevent overlapping requests: an old invalid-token response must not
  // overwrite a successful reveal from a newer poll.
  if(!token || revealPolling)return;
  const requestToken=token;
  const generation=revealGeneration;
  revealPolling=true;
  try{
    const x=await post('/api/diag/reveal/status',{token:requestToken});
    if(generation!==revealGeneration || token!==requestToken)return;
    if(x.state==='confirmed'){
      // Claim the confirmation before the next timer can start a second poll.
      const y=await post('/api/diag/reveal/consume',{token:requestToken});
      if(generation!==revealGeneration || token!==requestToken)return;
      token='';
      $('secret').textContent='Wi-Fi: '+y.ssid+'\nPassword: '+y.password+
        '\nSetup URL: http://192.168.4.1';
      set('revealMessage','Confirmed! Record the setup information on the lamp label.');
      shownAt=Date.now();
    }else if(x.state==='invalid'){
      token='';
      set('revealMessage','Request expired; try again.');
    }
  }catch(e){
    if(generation===revealGeneration && token===requestToken){
      token='';
      set('revealMessage','Confirmation failed; try again.');
    }
  }finally{
    revealPolling=false;
  }
}
refresh();setInterval(refresh,1000);setInterval(pollReveal,1200);
</script></body></html>
)diaghtml";

