#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "secrets.h"

// ========== КОНФИГУРАЦИЯ ПИНОВ (L298N) ==========
const int motorPins[4] = {5, 13, 12, 14}; // IN1, IN2, IN3, IN4 (GPIO)
const int ENA = 4;  // D2 - ШИМ левого борта
const int ENB = 0;  // D3 - ШИМ правого борта

// ========== МАТРИЦА ДВИЖЕНИЙ ==========
const bool moveForward[4]  = {1,0,1,0};
const bool moveBackward[4] = {0,1,0,1};
const bool moveLeft[4]     = {1,0,0,1};  // Правый вперёд, левый назад
const bool moveRight[4]    = {0,1,1,0};  // Левый вперёд, правый назад
const bool moveStop[4]     = {0,0,0,0};

// ========== EEPROM СТРУКТУРА ==========
struct Settings {
  char wifiSSID[32];
  char wifiPass[64];
  char webUser[32];
  char webPass[32];
  uint8_t valid;
};

Settings settings;
const uint8_t VALID_MARKER = 0xAA;
ESP8266WebServer server(80);
int motorSpeed = 1023; // 0-1023
String serialBuffer = "";

// ========== EEPROM ==========
void loadSettings() {
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  if (settings.valid != VALID_MARKER) {
    // Используем значения из secrets.h при первой инициализации
    strncpy(settings.wifiSSID, WIFI_SSID, sizeof(settings.wifiSSID) - 1);
    settings.wifiSSID[sizeof(settings.wifiSSID) - 1] = '\0';
    
    strncpy(settings.wifiPass, WIFI_PASS, sizeof(settings.wifiPass) - 1);
    settings.wifiPass[sizeof(settings.wifiPass) - 1] = '\0';
    
    strncpy(settings.webUser, WEB_LOGIN, sizeof(settings.webUser) - 1);
    settings.webUser[sizeof(settings.webUser) - 1] = '\0';
    
    strncpy(settings.webPass, WEB_PASS, sizeof(settings.webPass) - 1);
    settings.webPass[sizeof(settings.webPass) - 1] = '\0';
    
    settings.valid = VALID_MARKER;
    saveSettings();
    Serial.println(F("[EEPROM] Initialized with secrets.h values"));
  }
  EEPROM.end();
}

void saveSettings() {
  EEPROM.begin(512);
  EEPROM.put(0, settings);
  EEPROM.commit();
  EEPROM.end();
  Serial.println(F("[EEPROM] Settings saved"));
}

// ========== МОТОРЫ ==========
void applyMotorState(const bool* state) {
  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    analogWrite(motorPins[i], state[i] ? motorSpeed : 0);
  }
}

void stopMotors() {
  applyMotorState(moveStop);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ========== SERIAL CLI ==========
bool checkAuth() {
  if (!server.authenticate(settings.webUser, settings.webPass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd == "help") {
    Serial.println(F("\n========= ESPBOT CLI ========="));
    Serial.println(F("help          - This help"));
    Serial.println(F("status        - System status"));
    Serial.println(F("pinout        - Pin configuration"));
    Serial.println(F("login <u> <p> - Change web credentials"));
    Serial.println(F("wifi <s> <p>  - Change WiFi AP (reboot)"));
    Serial.println(F("backup        - EEPROM hex dump"));
    Serial.println(F("speed <0-1023>- Set motor PWM"));
    Serial.println(F("reboot        - Restart ESP"));
    Serial.println(F("==============================\n"));
  }
  else if (cmd == "status") {
    Serial.println(F("\n====== STATUS ======"));
    Serial.printf("WiFi SSID : %s\n", settings.wifiSSID);
    Serial.printf("WiFi Pass : %s\n", settings.wifiPass);
    Serial.printf("Web User  : %s\n", settings.webUser);
    Serial.printf("Web Pass  : %s\n", settings.webPass);
    Serial.printf("AP IP     : %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("ADC A0    : %d\n", analogRead(A0));
    Serial.printf("Speed PWM : %d\n", motorSpeed);
    Serial.println(F("====================\n"));
  }
  else if (cmd == "pinout") {
    Serial.println(F("\n====== PINOUT ======"));
    Serial.printf("IN1 (GPIO %d) - Left  Forward\n", motorPins[0]);
    Serial.printf("IN2 (GPIO %d) - Left  Backward\n", motorPins[1]);
    Serial.printf("IN3 (GPIO %d) - Right Forward\n", motorPins[2]);
    Serial.printf("IN4 (GPIO %d) - Right Backward\n", motorPins[3]);
    Serial.printf("ENA (GPIO %d) - Left  PWM\n", ENA);
    Serial.printf("ENB (GPIO %d) - Right PWM\n", ENB);
    Serial.printf("ADC (A0)      - Sensor\n");
    Serial.println(F("====================\n"));
  }
  else if (cmd.startsWith("login ")) {
    int sp = cmd.indexOf(' ', 6);
    if (sp > 6) {
      cmd.substring(6, sp).toCharArray(settings.webUser, 32);
      cmd.substring(sp+1).toCharArray(settings.webPass, 32);
      saveSettings();
      Serial.println(F("[AUTH] Web credentials updated"));
    } else Serial.println(F("Usage: login <user> <pass>"));
  }
  else if (cmd.startsWith("wifi ")) {
    int sp = cmd.indexOf(' ', 5);
    if (sp > 5) {
      cmd.substring(5, sp).toCharArray(settings.wifiSSID, 32);
      cmd.substring(sp+1).toCharArray(settings.wifiPass, 64);
      saveSettings();
      Serial.println(F("[WIFI] AP updated. Reboot to apply."));
    } else Serial.println(F("Usage: wifi <ssid> <pass>"));
  }
  else if (cmd == "backup") {
    EEPROM.begin(512);
    Serial.println(F("\n==== EEPROM BACKUP (512 bytes) ===="));
    for (int i = 0; i < 512; i++) {
      char buf[4];
      sprintf(buf, "%02X ", EEPROM.read(i));
      Serial.print(buf);
      if ((i+1) % 16 == 0) Serial.println();
    }
    Serial.println(F("\n==================================\n"));
    EEPROM.end();
  }
  else if (cmd.startsWith("speed ")) {
    int spd = cmd.substring(6).toInt();
    if (spd >= 0 && spd <= 1023) {
      motorSpeed = spd;
      Serial.printf("[MOTOR] Speed = %d\n", motorSpeed);
    } else Serial.println(F("Range: 0-1023"));
  }
  else if (cmd == "reboot") {
    Serial.println(F("[SYS] Rebooting..."));
    ESP.restart();
  }
  else if (cmd.length() > 0) {
    Serial.println(F("Unknown command. Type 'help'"));
  }
}

void processSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        Serial.println("> " + serialBuffer);
        handleCommand(serialBuffer);
        serialBuffer = "";
      }
    } else if (c >= 32 && c < 127) {
      serialBuffer += c;
    }
  }
}

// ========== WEB HANDLERS ==========
void handleMove() {
  if (!checkAuth()) return;
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed"); return;
  }
  String dir = server.arg("dir"), act = server.arg("action");
  if (act == "start") {
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
    if      (dir == "forward")  applyMotorState(moveForward);
    else if (dir == "backward") applyMotorState(moveBackward);
    else if (dir == "left")     applyMotorState(moveLeft);
    else if (dir == "right")    applyMotorState(moveRight);
  } else if (act == "stop") stopMotors();
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  if (!checkAuth()) return;
  String json = "{\"adc\":" + String(analogRead(A0)) + 
                ",\"speed\":" + String(motorSpeed) + "}";
  server.send(200, "application/json", json);
}

void handleSpeed() {
  if (!checkAuth()) return;
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed"); return;
  }
  int spd = server.arg("val").toInt();
  if (spd >= 0 && spd <= 1023) {
    motorSpeed = spd;
    server.send(200, "text/plain", "OK");
  } else server.send(400, "text/plain", "Invalid");
}

void handleRoot() {
  if (!checkAuth()) return;
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
<title>ESPBOT v2.0</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;user-select:none;-webkit-tap-highlight-color:transparent}
body{min-height:100vh;background:radial-gradient(circle at 20% 30%,#0a0f1a,#010101);display:flex;justify-content:center;align-items:center;font-family:'Courier New',monospace;padding:20px}
.glass{background:rgba(10,20,30,.65);backdrop-filter:blur(12px);border-radius:2rem;border:1px solid rgba(0,255,255,.4);box-shadow:0 0 30px rgba(0,255,255,.2);padding:1.5rem;width:100%;max-width:550px}
h1{font-size:1.6rem;font-weight:800;text-align:center;letter-spacing:3px;background:linear-gradient(135deg,#0ff,#3cc);-webkit-background-clip:text;background-clip:text;color:transparent;text-shadow:0 0 8px rgba(0,255,255,.3);margin-bottom:.2rem}
.sub{text-align:center;color:#6f9fbf;font-size:.65rem;border-bottom:1px solid #2c5f6e;display:inline-block;margin:0 auto 1.2rem auto;padding-bottom:4px;width:100%}
.adc-card,.speed-card{background:#07121b;border-radius:1rem;padding:.6rem;margin-bottom:1rem;text-align:center;border:1px solid #0ff3}
.adc-label,.speed-label{font-size:.65rem;letter-spacing:1px;color:#6f9fbf}
.adc-value{font-size:2.2rem;font-weight:800;color:#0ff;text-shadow:0 0 6px #0ff;line-height:1}
.speed-slider{width:100%;-webkit-appearance:none;height:8px;background:#0a1a2a;border-radius:5px;outline:none;margin-top:8px}
.speed-slider::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;background:#0ff;border-radius:50%;cursor:pointer;box-shadow:0 0 10px #0ff}
.controls{display:grid;grid-template-columns:repeat(3,1fr);gap:.8rem;margin:1.2rem 0}
.ctrl-btn{background:rgba(0,30,40,.8);border:1.5px solid #0ff;color:#0ff;font-family:'Courier New',monospace;font-weight:bold;font-size:1.3rem;padding:.8rem 0;border-radius:60px;text-align:center;cursor:pointer;transition:.05s linear}
.ctrl-btn:active{transform:scale(.96);background:#0ff2;box-shadow:0 0 16px #0ff}
.footer{font-size:.55rem;text-align:center;margin-top:1.2rem;color:#2f7f8f;border-top:1px solid #1e4a5a;padding-top:.8rem;line-height:1.6}
.footer a{color:#0ff;text-decoration:none}
@media (max-width:480px){.ctrl-btn{font-size:1rem;padding:.6rem 0}.adc-value{font-size:1.8rem}.glass{padding:1rem}}
</style>
</head>
<body>
<div class="glass">
<h1>⚡ ESPBOT ⚡</h1>
<div style="text-align:center"><span class="sub">[ L298N DRIVE v2.0 ]</span></div>
<div class="adc-card">
<div class="adc-label">ADC A0 SENSOR</div>
<div class="adc-value" id="adcVal">---</div>
<div style="font-size:.6rem">0‑1023</div>
</div>
<div class="speed-card">
<div class="speed-label">MOTOR PWM: <span id="speedVal">1023</span></div>
<input type="range" min="0" max="1023" value="1023" class="speed-slider" id="speedSlider">
</div>
<div class="controls">
<div></div>
<div class="ctrl-btn" id="btnForward">▲ FWD</div>
<div></div>
<div class="ctrl-btn" id="btnLeft">◀ L</div>
<div class="ctrl-btn" id="btnBack">▼ BWD</div>
<div class="ctrl-btn" id="btnRight">R ▶</div>
</div>
<div class="footer">
HOLD → MOVE &nbsp;|&nbsp; RELEASE → STOP<br>
&copy; sa &nbsp;|&nbsp; <a href="http://www.juniorgenius.ru/it" target="_blank">www.juniorgenius.ru/it</a>
</div>
</div>
<script>
async function cmd(dir,act){
  try{
    let fd=new URLSearchParams();
    fd.append('dir',dir);fd.append('action',act);
    await fetch('/move',{method:'POST',body:fd});
  }catch(e){}
}
async function updADC(){
  try{
    let r=await fetch('/status'),d=await r.json();
    document.getElementById('adcVal').innerText=d.adc;
    document.getElementById('speedVal').innerText=d.speed;
    document.getElementById('speedSlider').value=d.speed;
  }catch(e){}
}
document.getElementById('speedSlider').addEventListener('input',async e=>{
  document.getElementById('speedVal').innerText=e.target.value;
  let fd=new URLSearchParams();fd.append('val',e.target.value);
  try{await fetch('/speed',{method:'POST',body:fd});}catch(x){}
});
const btns=[
  {id:'btnForward',dir:'forward'},{id:'btnBack',dir:'backward'},
  {id:'btnLeft',dir:'left'},{id:'btnRight',dir:'right'}
];
for(let b of btns){
  let el=document.getElementById(b.id);if(!el)continue;
  let start=e=>{e.preventDefault();cmd(b.dir,'start');};
  let stop=()=>cmd(b.dir,'stop');
  el.addEventListener('touchstart',start,{passive:false});
  el.addEventListener('touchend',stop);
  el.addEventListener('touchcancel',stop);
  el.addEventListener('mousedown',start);
  el.addEventListener('mouseup',stop);
  el.addEventListener('mouseleave',stop);
}
setInterval(updADC,500);updADC();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

// ========== OTA ==========
void setupOTA() {
  ArduinoOTA.setHostname("ESPBOT");
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([](){
    stopMotors();
    Serial.println(F("[OTA] Start"));
  });
  ArduinoOTA.onEnd([](){ Serial.println(F("\n[OTA] End")); });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t){
    Serial.printf("[OTA] %u%%\r", (p/(t/100)));
  });
  ArduinoOTA.onError([](ota_error_t e){
    Serial.printf("[OTA] Error[%u]\n", e);
  });
  ArduinoOTA.begin();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("\n\n"));
  Serial.println(F("╔══════════════════════════════════╗"));
  Serial.println(F("║   ESPBOT v2.0 — HI-TECH CTRL     ║"));
  Serial.println(F("║   (c) sa | juniorgenius.ru/it    ║"));
  Serial.println(F("╚══════════════════════════════════╝"));

  loadSettings();

  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(settings.wifiSSID, settings.wifiPass);
  Serial.printf("[WIFI] AP '%s' started\n", settings.wifiSSID);
  Serial.printf("[WIFI] IP: %s\n", WiFi.softAPIP().toString().c_str());

  setupOTA();

  server.on("/", handleRoot);
  server.on("/move", HTTP_POST, handleMove);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/speed", HTTP_POST, handleSpeed);
  server.onNotFound([](){ server.send(404, "text/plain", "404"); });
  server.begin();
  Serial.println(F("[HTTP] Server started"));
  Serial.println(F("[SYS] Type 'help' for CLI\n"));
}

void loop() {
  processSerial();
  server.handleClient();
  ArduinoOTA.handle();
}