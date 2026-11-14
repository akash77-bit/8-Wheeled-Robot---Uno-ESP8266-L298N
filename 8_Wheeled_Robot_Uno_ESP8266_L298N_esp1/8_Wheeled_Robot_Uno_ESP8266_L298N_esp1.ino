/*
  8-Wheeled Robot - ESP8266 Wi-Fi bridge
  Phone/app --Wi-Fi--> ESP8266 --Serial "F180"--> Arduino Uno
  DEMO = true: sends a test sequence to the Uno at start (for the simulator).
  Set DEMO = false on the real robot.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* AP_SSID = "Robot8W";
const char* AP_PASS = "robot1234";
const bool DEMO = true;

ESP8266WebServer server(80);

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Robot8W</title>
<style>
body{font-family:sans-serif;text-align:center;background:#1e1e2e;color:#fff;margin:0;padding:16px}
.pad{display:grid;grid-template-columns:repeat(3,90px);gap:10px;justify-content:center;margin:20px 0}
button{height:90px;font-size:32px;border:0;border-radius:14px;background:#3b82f6;color:#fff;touch-action:none}
#stop{background:#ef4444} input{width:80%}
</style></head><body>
<h2>8-Wheeled Robot</h2>
<div class="pad">
<span></span><button data-c="F">&#9650;</button><span></span>
<button data-c="L">&#9664;</button><button id="stop" data-c="S">&#9632;</button><button data-c="R">&#9654;</button>
<span></span><button data-c="B">&#9660;</button><span></span>
</div>
<p>Speed: <b id="v">150</b></p>
<input type="range" id="spd" min="0" max="255" value="150">
<p id="st">Ready</p>
<script>
const spd=document.getElementById('spd'),v=document.getElementById('v'),st=document.getElementById('st');
spd.oninput=()=>v.textContent=spd.value;
function send(c){fetch('/'+c+'?s='+spd.value).then(r=>r.text()).then(t=>st.textContent=t).catch(()=>st.textContent='No connection');}
document.querySelectorAll('button').forEach(b=>{
  b.onpointerdown=()=>send(b.dataset.c);
  b.onpointerup=b.onpointerleave=()=>{if(b.dataset.c!=='S')send('S');};
});
const keys={ArrowUp:'F',w:'F',ArrowDown:'B',s:'B',ArrowLeft:'L',a:'L',ArrowRight:'R',d:'R',' ':'S'};
let down=null;
document.onkeydown=e=>{const c=keys[e.key];if(c&&down!==c){down=c;send(c);}};
document.onkeyup=e=>{if(keys[e.key]){down=null;send('S');}};
</script></body></html>
)rawliteral";

void sendToUno(const String& cmd) {
  Serial.print(cmd);
  Serial.print('\n');
}

void handleMove(char c) {
  String cmd = String(c) + server.arg("s");
  sendToUno(cmd);
  server.send(200, "text/plain", "Sent " + cmd);
}

struct Step { const char* cmd; unsigned long ms; };
Step demo[] = {
  {"F200", 3000}, {"S", 1000}, {"B150", 3000}, {"S", 1000},
  {"L180", 2000}, {"R180", 2000}, {"F80", 3000}, {"S", 0}
};
const int DEMO_STEPS = sizeof(demo) / sizeof(demo[0]);
int demoIndex = 0;
unsigned long demoNext = 0;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/F", []() { handleMove('F'); });
  server.on("/B", []() { handleMove('B'); });
  server.on("/L", []() { handleMove('L'); });
  server.on("/R", []() { handleMove('R'); });
  server.on("/S", []() { handleMove('S'); });
  server.begin();
  demoNext = millis() + 2000;
}

void loop() {
  server.handleClient();
  if (DEMO && demoIndex < DEMO_STEPS && millis() >= demoNext) {
    sendToUno(demo[demoIndex].cmd);
    demoNext = millis() + demo[demoIndex].ms;
    demoIndex++;
  }
}