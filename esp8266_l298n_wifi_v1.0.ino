#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DNSServer.h>

// ========== FIRMWARE INFO ==========
#define FW_VERSION "1.0"
#define EEPROM_SIZE 512

const char CFG_MAGIC[4] = {'H', 'T', 'R', '1'};

struct Config {
  char magic[4];
  char webLogin[24];
  char webPass[24];
  char apSsid[32];
  char apPass[64];
  uint16_t crc;
};

Config cfg;

ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

String serialBuffer;

// ========== ПИНЫ ДЛЯ ДВИГАТЕЛЕЙ ==========
const int motorPins[4] = {15, 13, 12, 14};

const bool moveForward[4]  = {1, 0, 1, 0};
const bool moveBackward[4] = {0, 1, 0, 1};
const bool moveLeft[4]     = {0, 1, 1, 0};
const bool moveRight[4]    = {1, 0, 0, 1};
const bool moveStop[4]     = {0, 0, 0, 0};

// ========== WEB PAGES ==========
static const char PAGE_LOGIN[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
<title>ESPLOGIN</title>
<style>
*{
margin:0;
padding:0;
box-sizing:border-box;
user-select:none;
-webkit-tap-highlight-color:transparent;
}
body{
min-height:100vh;
display:flex;
justify-content:center;
align-items:center;
background:radial-gradient(circle at 20% 30%, #0a0f1a, #010101);
font-family:'Courier New', monospace;
padding:20px;
}
.glass{
width:100%;
max-width:420px;
background:rgba(10,20,30,.65);
backdrop-filter:blur(12px);
border:1px solid rgba(0,255,255,.4);
border-radius:2rem;
box-shadow:0 0 30px rgba(0,255,255,.2);
padding:1.5rem;
}
h1{
font-size:1.6rem;
font-weight:800;
text-align:center;
letter-spacing:3px;
background:linear-gradient(135deg,#0ff,#3cc);
-webkit-background-clip:text;
background-clip:text;
color:transparent;
text-shadow:0 0 8px rgba(0,255,255,.3);
margin-bottom:.2rem;
}
.center{text-align:center;}
.sub{
color:#6f9fbf;
font-size:.65rem;
border-bottom:1px solid #2c5f6e;
display:inline-block;
padding-bottom:4px;
margin-bottom:1rem;
}
.error{
display:none;
margin:0 0 12px;
padding:10px;
border:1px solid rgba(255,80,80,.7);
border-radius:10px;
color:#f88;
text-align:center;
font-size:.75rem;
background:rgba(70,0,0,.25);
}
label{
display:block;
margin:12px 0 6px;
color:#6f9fbf;
font-size:.7rem;
letter-spacing:1px;
}
input{
width:100%;
background:#07121b;
border:1px solid rgba(0,255,255,.35);
border-radius:12px;
color:#0ff;
padding:12px;
outline:none;
font-family:inherit;
font-size:1rem;
}
input:focus{
box-shadow:0 0 12px rgba(0,255,255,.35);
}
button{
width:100%;
margin-top:18px;
background:rgba(0,30,40,.8);
border:1.5px solid #0ff;
color:#0ff;
padding:12px;
border-radius:14px;
font-family:inherit;
font-weight:bold;
letter-spacing:2px;
cursor:pointer;
}
button:active{
transform:scale(.98);
box-shadow:0 0 16px #0ff;
}
.footer{
margin-top:14px;
text-align:center;
color:#2f7f8f;
font-size:.6rem;
border-top:1px solid #1e4a5a;
padding-top:10px;
}
</style>
</head>
<body>
<div class="glass">
<h1>⚡ ESPBOT⚡</h1>
<div class="center"><span class="sub">[ ESP8266 DRIVE ]</span></div>
<div class="error" id="err">ACCESS DENIED</div>
<form method="POST" action="/login" autocomplete="off">
<label>LOGIN</label>
<input type="text" name="login" required autofocus>
<label>PASSWORD</label>
<input type="password" name="password" required>
<button type="submit">ENTER SYSTEM</button>
</form>
<div class="footer">HI-TECH CTRL v1.0 · © sa * juniorgenius.ru</div>
</div>
<script>
if (new URLSearchParams(location.search).has('err')) {
  document.getElementById('err').style.display = 'block';
}
</script>
</body>
</html>)rawliteral";

static const char PAGE_CTRL[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
<title>HI-TECH CTRL</title>
<style>
*{
margin:0;
padding:0;
box-sizing:border-box;
user-select:none;
-webkit-tap-highlight-color:transparent;
}
body{
min-height:100vh;
background:radial-gradient(circle at 20% 30%, #0a0f1a, #010101);
display:flex;
justify-content:center;
align-items:center;
font-family:'Courier New', monospace;
padding:20px;
}
.glass{
background:rgba(10,20,30,.65);
backdrop-filter:blur(12px);
border-radius:2rem;
border:1px solid rgba(0,255,255,.4);
box-shadow:0 0 30px rgba(0,255,255,.2);
padding:1.5rem;
width:100%;
max-width:550px;
}
.top{
display:flex;
justify-content:space-between;
align-items:center;
margin-bottom:10px;
gap:10px;
}
.badge{
color:#6f9fbf;
font-size:.65rem;
border:1px solid #2c5f6e;
padding:4px 8px;
border-radius:999px;
white-space:nowrap;
}
.logout{
color:#f77;
text-decoration:none;
font-size:.7rem;
border:1px solid rgba(255,80,80,.5);
padding:4px 8px;
border-radius:999px;
white-space:nowrap;
}
h1{
font-size:1.6rem;
font-weight:800;
text-align:center;
letter-spacing:3px;
background:linear-gradient(135deg,#0ff,#3cc);
-webkit-background-clip:text;
background-clip:text;
color:transparent;
text-shadow:0 0 8px rgba(0,255,255,.3);
margin-bottom:.2rem;
}
.center{text-align:center;}
.sub{
text-align:center;
color:#6f9fbf;
font-size:.65rem;
border-bottom:1px solid #2c5f6e;
display:inline-block;
width:auto;
margin:0 auto 1.2rem auto;
padding-bottom:4px;
}
.adc-card{
background:#07121b;
border-radius:1rem;
padding:.6rem;
margin-bottom:1.2rem;
text-align:center;
border:1px solid rgba(0,255,255,.2);
}
.adc-label{
font-size:.65rem;
letter-spacing:1px;
color:#6f9fbf;
}
.adc-value{
font-size:2.2rem;
font-weight:800;
color:#0ff;
text-shadow:0 0 6px #0ff;
line-height:1;
}
.adc-range{
font-size:.6rem;
color:#4f8fa5;
margin-top:4px;
}
.controls{
display:grid;
grid-template-columns:repeat(3,1fr);
gap:.8rem;
margin:1.2rem 0;
}
.ctrl-btn{
background:rgba(0,30,40,.8);
border:1.5px solid #0ff;
color:#0ff;
font-family:'Courier New', monospace;
font-weight:bold;
font-size:1.3rem;
padding:.8rem 0;
border-radius:60px;
text-align:center;
cursor:pointer;
transition:.05s linear;
}
.ctrl-btn:active{
transform:scale(.96);
background:rgba(0,255,255,.13);
box-shadow:0 0 16px #0ff;
}
.sys{
background:#07121b;
border:1px solid rgba(0,255,255,.2);
border-radius:10px;
padding:8px;
font-size:.6rem;
color:#6f9fbf;
text-align:center;
overflow-wrap:anywhere;
margin-top:10px;
}
.footer{
font-size:.55rem;
text-align:center;
margin-top:1.2rem;
color:#2f7f8f;
border-top:1px solid #1e4a5a;
padding-top:.8rem;
line-height:1.4;
}
@media (max-width:480px){
.ctrl-btn{font-size:1rem;padding:.6rem 0;}
.adc-value{font-size:1.8rem;}
.glass{padding:1rem;}
}
</style>
</head>
<body>
<div class="glass">
<div class="top">
<span class="badge">HI-TECH CTRL v1.0</span>
<a class="logout" href="/logout">LOGOUT</a>
</div>

<h1>⚡ ESPBOT⚡</h1>
<div class="center"><span class="sub">[ ESP8266 DRIVE ]</span></div>

<div class="adc-card">
<div class="adc-label">ADC A0</div>
<div class="adc-value" id="adcVal">---</div>
<div class="adc-range">0-1023</div>
</div>

<div class="controls">
<div></div>
<div class="ctrl-btn" id="btnForward">▲ FWD</div>
<div></div>

<div class="ctrl-btn" id="btnLeft">◀ L</div>
<div class="ctrl-btn" id="btnBack">▼ BWD</div>
<div class="ctrl-btn" id="btnRight">R ▶</div>
</div>

<div class="sys" id="sysLine">SYS: ---</div>

<div class="footer">
HOLD → MOVE | RELEASE → STOP<br>
© sa * juniorgenius.ru
</div>
</div>

<script>
function $(id) {
  return document.getElementById(id);
}

async function cmd(dir, act) {
  try {
    const fd = new URLSearchParams();
    fd.append('dir', dir);
    fd.append('action', act);
    const r = await fetch('/move', {
      method: 'POST',
      body: fd
    });
    if (r.status === 401) location.href = '/login';
  } catch (e) {}
}

async function updStatus() {
  try {
    const r = await fetch('/status');
    if (r.status === 401) {
      location.href = '/login';
      return;
    }
    const d = await r.json();
    $('adcVal').innerText = d.adc;
    $('sysLine').innerText =
      'SSID: ' + d.ssid +
      ' | IP: ' + d.ip +
      ' | UPTIME: ' + d.uptime + 's' +
      ' | HEAP: ' + d.heap +
      ' | CLIENTS: ' + d.clients;
  } catch (e) {}
}

const btns = [
  {id:'btnForward', dir:'forward'},
  {id:'btnBack', dir:'backward'},
  {id:'btnLeft', dir:'left'},
  {id:'btnRight', dir:'right'}
];

for (const b of btns) {
  const el = $(b.id);
  if (!el) continue;

  const start = (e) => {
    e.preventDefault();
    cmd(b.dir, 'start');
  };

  const stop = () => {
    cmd(b.dir, 'stop');
  };

  el.addEventListener('contextmenu', e => e.preventDefault());
  el.addEventListener('touchstart', start, {passive:false});
  el.addEventListener('touchend', stop);
  el.addEventListener('touchcancel', stop);
  el.addEventListener('mousedown', start);
  el.addEventListener('mouseup', stop);
  el.addEventListener('mouseleave', stop);
}

setInterval(updStatus, 1000);
updStatus();
</script>
</body>
</html>)rawliteral";

// ========== EEPROM / CONFIG ==========
uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else crc <<= 1;
    }
  }
  return crc;
}

void copyStr(char* dst, size_t size, const String& src) {
  if (size == 0) return;
  if (src.length() == 0) {
    dst[0] = 0;
    return;
  }
  src.toCharArray(dst, size);
  dst[size - 1] = 0;
}

void updateCrc() {
  cfg.crc = crc16((const uint8_t*)&cfg, sizeof(cfg) - sizeof(cfg.crc));
}

void setDefaultConfig() {
  memcpy(cfg.magic, CFG_MAGIC, 4);
  copyStr(cfg.webLogin, sizeof(cfg.webLogin), "admin");
  copyStr(cfg.webPass, sizeof(cfg.webPass), "admin");
  copyStr(cfg.apSsid, sizeof(cfg.apSsid), "NEXUS-HITECH");
  copyStr(cfg.apPass, sizeof(cfg.apPass), "adminadmin");
  updateCrc();
}

void saveConfig() {
  updateCrc();
  EEPROM.put(0, cfg);
  EEPROM.commit();
}

bool loadConfig() {
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, cfg);

  uint16_t calc = crc16((const uint8_t*)&cfg, sizeof(cfg) - sizeof(cfg.crc));

  if (memcmp(cfg.magic, CFG_MAGIC, 4) != 0 || calc != cfg.crc) {
    setDefaultConfig();
    saveConfig();
    return false;
  }

  return true;
}

// ========== WIFI AP ==========
void applyWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPdisconnect(true);
  delay(100);

  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  bool ok = false;

  if (strlen(cfg.apPass) == 0) {
    ok = WiFi.softAP(cfg.apSsid);
  } else {
    ok = WiFi.softAP(cfg.apSsid, cfg.apPass);
  }

  if (ok) Serial.println(F("WiFi AP started"));
  else Serial.println(F("WiFi AP FAILED"));

  Serial.print(F("AP SSID: "));
  Serial.println(cfg.apSsid);

  Serial.print(F("AP password: "));
  if (strlen(cfg.apPass) == 0) Serial.println(F("[open]"));
  else Serial.println(F("********"));

  Serial.print(F("AP IP: "));
  Serial.println(WiFi.softAPIP().toString());

  Serial.println(F("Web: http://192.168.4.1"));

  dnsServer.stop();
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

// ========== MOTORS ==========
void applyMotorState(const bool* state) {
  for (int i = 0; i < 4; i++) {
    digitalWrite(motorPins[i], state[i] ? HIGH : LOW);
  }
}

void stopMotors() {
  applyMotorState(moveStop);
}

// ========== WEB AUTH ==========
uint32_t fnv1a(const String& s) {
  uint32_t h = 0x811c9dc5;
  for (size_t i = 0; i < s.length(); i++) {
    h ^= (uint8_t)s[i];
    h *= 0x01000193;
  }
  return h;
}

String authToken() {
  String raw = String(cfg.webLogin) + ":" +
               String(cfg.webPass) + ":" +
               String(ESP.getChipId(), HEX) + ":" +
               FW_VERSION;
  return String(fnv1a(raw), HEX);
}

String getCookieValue(const String& name) {
  if (!server.hasHeader("Cookie")) return "";

  String cookie = server.header("Cookie");
  int pos = 0;

  while (pos < (int)cookie.length()) {
    int eq = cookie.indexOf('=', pos);
    if (eq < 0) break;

    String key = cookie.substring(pos, eq);
    key.trim();

    int semicolon = cookie.indexOf(';', eq);
    String value;
    if (semicolon < 0) value = cookie.substring(eq + 1);
    else value = cookie.substring(eq + 1, semicolon);
    value.trim();

    if (key == name) return value;

    if (semicolon < 0) break;
    pos = semicolon + 1;
  }

  return "";
}

bool isAuthed() {
  String token = getCookieValue("HTAUTH");
  return token.length() > 0 && token.equals(authToken());
}

// ========== WEB HANDLERS ==========
void handleRoot() {
  if (!isAuthed()) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PAGE_CTRL);
}

void handleLoginGet() {
  if (isAuthed()) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    return;
  }

  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", PAGE_LOGIN);
}

void handleLoginPost() {
  String login = server.arg("login");
  String pass = server.arg("password");
  login.trim();
  pass.trim();

  if (login == String(cfg.webLogin) && pass == String(cfg.webPass)) {
    String cookie = String("HTAUTH=") + authToken() + "; Path=/; Max-Age=86400; HttpOnly";
    server.sendHeader("Location", "/");
    server.sendHeader("Set-Cookie", cookie);
    server.send(302, "text/plain", "");
  } else {
    server.sendHeader("Location", "/login?err=1");
    server.send(302, "text/plain", "");
  }
}

void handleLogout() {
  server.sendHeader("Location", "/login");
  server.sendHeader("Set-Cookie", "HTAUTH=deleted; Path=/; Max-Age=0; HttpOnly");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  if (!isAuthed()) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\"}");
    return;
  }

  int adcValue = analogRead(A0);

  String json = "{";

  json += "\"adc\":";
  json += String(adcValue);

  json += ",\"uptime\":";
  json += String(millis() / 1000);

  json += ",\"heap\":";
  json += String(ESP.getFreeHeap());

  json += ",\"ssid\":\"";
  json += cfg.apSsid;
  json += "\"";

  json += ",\"ip\":\"";
  json += WiFi.softAPIP().toString();
  json += "\"";

  json += ",\"clients\":";
  json += String(WiFi.softAPgetStationNum());

  json += "}";

  server.send(200, "application/json", json);
}

void handleMove() {
  if (!isAuthed()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String direction = server.arg("dir");
  String action = server.arg("action");

  if (action == "start") {
    bool known = true;

    if (direction == "forward") applyMotorState(moveForward);
    else if (direction == "backward") applyMotorState(moveBackward);
    else if (direction == "left") applyMotorState(moveLeft);
    else if (direction == "right") applyMotorState(moveRight);
    else known = false;

    if (!known) stopMotors();
  } else if (action == "stop") {
    stopMotors();
  } else {
    stopMotors();
    server.send(400, "text/plain", "Bad action");
    return;
  }

  server.send(200, "text/plain", "OK");
}

void handleNotFound() {
  if (server.uri() == "/favicon.ico") {
    server.send(204, "text/plain", "");
    return;
  }

  if (!isAuthed()) {
    server.sendHeader("Location", "/login");
    server.send(302, "text/plain", "");
    return;
  }

  server.send(404, "text/plain", "404");
}

// ========== SERIAL CONSOLE ==========
void printHelp() {
  Serial.println();
  Serial.println(F("=== ESPHITECH v1.0 serial console ==="));
  Serial.println(F("help                                     - this help"));
  Serial.println(F("status                                   - system status"));
  Serial.println(F("pinout                                   - motor pinout"));
  Serial.println(F("setlogin <new_login> <new_password>      - change web login/password"));
  Serial.println(F("setwifi <ssid> <password>                - change AP WiFi password"));
  Serial.println(F("setwifi <ssid>                           - set open AP"));
  Serial.println(F("setwifi <ssid> open                      - set open AP"));
  Serial.println(F("backup                                   - EEPROM hex backup"));
  Serial.println(F("reset                                    - factory reset"));
  Serial.println(F("reboot                                   - reboot ESP8266"));
  Serial.println();
}

void printStatus() {
  Serial.println(F("=== STATUS ==="));
  Serial.println(F("Firmware: NEXUS-HITECH v" FW_VERSION));
  Serial.print(F("Chip ID: "));
  Serial.println(ESP.getChipId(), HEX);
  Serial.print(F("Uptime s: "));
  Serial.println(millis() / 1000);
  Serial.print(F("Free heap: "));
  Serial.println(ESP.getFreeHeap());
  Serial.print(F("ADC A0: "));
  Serial.println(analogRead(A0));

  Serial.println(F("WiFi mode: AP"));
  Serial.print(F("AP SSID: "));
  Serial.println(cfg.apSsid);
  Serial.print(F("AP password: "));
  Serial.println(F("********"));
  Serial.print(F("AP IP: "));
  Serial.println(WiFi.softAPIP().toString());
  Serial.print(F("AP MAC: "));
  Serial.println(WiFi.softAPmacAddress());
  Serial.print(F("AP clients: "));
  Serial.println(WiFi.softAPgetStationNum());

  Serial.print(F("Web login: "));
  Serial.println(cfg.webLogin);
  Serial.println(F("Web password: ********"));
  Serial.println();
}

void printPinout() {
  Serial.println(F("=== PINOUT ==="));

  for (int i = 0; i < 4; i++) {
    Serial.print(F("M"));
    Serial.print(i + 1);
    Serial.print(F(" -> GPIO"));
    Serial.println(motorPins[i]);
  }

  Serial.println(F("Logic table: M1 M2 M3 M4"));
  Serial.printf("FORWARD : %d %d %d %d\n", moveForward[0], moveForward[1], moveForward[2], moveForward[3]);
  Serial.printf("BACKWARD: %d %d %d %d\n", moveBackward[0], moveBackward[1], moveBackward[2], moveBackward[3]);
  Serial.printf("LEFT    : %d %d %d %d\n", moveLeft[0], moveLeft[1], moveLeft[2], moveLeft[3]);
  Serial.printf("RIGHT   : %d %d %d %d\n", moveRight[0], moveRight[1], moveRight[2], moveRight[3]);
  Serial.printf("STOP    : %d %d %d %d\n", moveStop[0], moveStop[1], moveStop[2], moveStop[3]);

  Serial.println();
}

void backupEeprom() {
  static uint8_t buf[EEPROM_SIZE];

  for (int i = 0; i < EEPROM_SIZE; i++) {
    buf[i] = EEPROM.read(i);
  }

  uint16_t fullCrc = crc16(buf, EEPROM_SIZE);

  Serial.println(F("=== EEPROM BACKUP ==="));
  Serial.println(F("# ESPHITECH EEPROM hex dump"));
  Serial.print(F("SIZE: "));
  Serial.println(EEPROM_SIZE);
  Serial.printf("CRC16: %04X\n", (unsigned)fullCrc);

  for (int addr = 0; addr < EEPROM_SIZE; addr += 16) {
    Serial.printf("%04X:", (unsigned)addr);

    for (int j = 0; j < 16; j++) {
      if (addr + j < EEPROM_SIZE) Serial.printf(" %02X", (unsigned)buf[addr + j]);
      else Serial.print("   ");
    }

    Serial.print(" |");

    for (int j = 0; j < 16; j++) {
      if (addr + j < EEPROM_SIZE) {
        uint8_t uc = buf[addr + j];
        Serial.print((uc >= 32 && uc <= 126) ? (char)uc : '.');
      }
    }

    Serial.println("|");
    yield();
  }

  Serial.println(F("=== END BACKUP ==="));
  Serial.println();
}

void cmdSetLogin(String args) {
  args.trim();

  int sp = args.indexOf(' ');
  if (sp < 0) {
    Serial.println(F("Usage: setlogin <new_login> <new_password>"));
    return;
  }

  String login = args.substring(0, sp);
  String pass = args.substring(sp + 1);
  login.trim();
  pass.trim();

  if (login.length() < 1 || login.length() >= sizeof(cfg.webLogin)) {
    Serial.print(F("Login length must be 1.."));
    Serial.println(sizeof(cfg.webLogin) - 1);
    return;
  }

  if (pass.length() < 1 || pass.length() >= sizeof(cfg.webPass)) {
    Serial.print(F("Password length must be 1.."));
    Serial.println(sizeof(cfg.webPass) - 1);
    return;
  }

  copyStr(cfg.webLogin, sizeof(cfg.webLogin), login);
  copyStr(cfg.webPass, sizeof(cfg.webPass), pass);
  saveConfig();

  Serial.println(F("Web login/password saved to EEPROM."));
  Serial.println(F("All active web sessions are now invalid."));
  Serial.println();
}

void cmdSetWifi(String args) {
  args.trim();

  if (args.length() == 0) {
    Serial.println(F("Usage: setwifi <ssid> <password>"));
    Serial.println(F("Usage: setwifi <ssid>"));
    Serial.println(F("Usage: setwifi <ssid> open"));
    return;
  }

  String ssid;
  String pass;

  int sp = args.indexOf(' ');
  if (sp < 0) {
    ssid = args;
    pass = "";
  } else {
    ssid = args.substring(0, sp);
    pass = args.substring(sp + 1);
    ssid.trim();
    pass.trim();
  }

  if (pass.equalsIgnoreCase("open") || pass == "-") {
    pass = "";
  }

  if (ssid.length() < 1 || ssid.length() >= sizeof(cfg.apSsid)) {
    Serial.print(F("SSID length must be 1.."));
    Serial.println(sizeof(cfg.apSsid) - 1);
    return;
  }

  if (pass.length() > 0 && (pass.length() < 8 || pass.length() >= sizeof(cfg.apPass))) {
    Serial.println(F("WiFi password length must be 8..63."));
    Serial.println(F("For open AP use: setwifi <ssid> open"));
    return;
  }

  copyStr(cfg.apSsid, sizeof(cfg.apSsid), ssid);
  copyStr(cfg.apPass, sizeof(cfg.apPass), pass);
  saveConfig();

  Serial.println(F("WiFi settings saved to EEPROM."));
  Serial.println(F("Restarting AP..."));
  applyWiFi();
  Serial.println();
}

void factoryReset() {
  setDefaultConfig();
  saveConfig();
  Serial.println(F("Factory reset done. Restarting AP..."));
  applyWiFi();
  Serial.println();
}

void processCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  String cmd = line;
  String args = "";

  int sp = line.indexOf(' ');
  if (sp >= 0) {
    cmd = line.substring(0, sp);
    args = line.substring(sp + 1);
  }

  cmd.trim();
  args.trim();
  cmd.toLowerCase();

  if (cmd == "help") {
    printHelp();
  } else if (cmd == "status") {
    printStatus();
  } else if (cmd == "pinout") {
    printPinout();
  } else if (cmd == "setlogin") {
    cmdSetLogin(args);
  } else if (cmd == "setwifi") {
    cmdSetWifi(args);
  } else if (cmd == "backup") {
    backupEeprom();
  } else if (cmd == "reset") {
    factoryReset();
  } else if (cmd == "reboot") {
    Serial.println(F("Rebooting..."));
    delay(100);
    ESP.restart();
  } else {
    Serial.println(F("Unknown command. Type: help"));
    Serial.println();
  }
}

void handleSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\n' || c == '\r') {
      String line = serialBuffer;
      serialBuffer = "";
      processCommand(line);
    } else if (c >= 32) {
      serialBuffer += c;
    }
  }
}

// ========== SETUP / LOOP ==========
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println(F("=== ESPHITECH v1.0 ==="));

  bool loaded = loadConfig();
  if (loaded) {
    Serial.println(F("EEPROM: config loaded"));
  } else {
    Serial.println(F("EEPROM: initialized with defaults"));
  }

  for (int i = 0; i < 4; i++) {
    pinMode(motorPins[i], OUTPUT);
    digitalWrite(motorPins[i], LOW);
  }

  applyWiFi();

  static const char* headerKeys[] = {"Cookie"};
  server.collectHeaders(headerKeys, 1);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_GET, handleLoginGet);
  server.on("/login", HTTP_POST, handleLoginPost);
  server.on("/logout", HTTP_GET, handleLogout);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/move", HTTP_POST, handleMove);

  server.on("/favicon.ico", HTTP_GET, []() {
    server.send(204, "text/plain", "");
  });

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println(F("HTTP server started"));
  printHelp();
}

void loop() {
  dnsServer.processNextRequests();
  server.handleClient();
  handleSerial();
}
