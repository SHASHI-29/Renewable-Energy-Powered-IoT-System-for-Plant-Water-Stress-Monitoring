#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

const char* ssid_sta = "SHASHI";
const char* password_sta = "12345678";

const char* ssid = "PLANT_STRESS";
const char* password = "12345678";

ESP8266WebServer server(80);

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SOIL_PIN A0
#define LDR_PIN D6
#define RELAY_PIN D5

float temperature = 0;
float humidity = 0;
int waterRaw = 0;
int waterPercent = 0;
int ldrState = 0;

String plantStatus = "NORMAL";
String ldrStatus = "Dim";
String pumpStatus = "OFF";
void handleRoot();
void handleData();
void setup() {
  Serial.begin(9600);

  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("IOT PLANT STRESS");
  lcd.setCursor(0, 1);
  lcd.print("MONITORING SYSTM");
  delay(3000);
  lcd.clear();

  dht.begin();

  // 🔥 NEW WIFI CONFIG
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPConfig(
    IPAddress(192,168,4,1),
    IPAddress(192,168,4,1),
    IPAddress(255,255,255,0)
  );
  WiFi.softAP(ssid, password);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(ssid_sta, password_sta);

  // Serial.print("Connecting to Internet");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to Internet");
  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());

  lcd.print("WiFi Started");
  delay(1000);
  lcd.clear();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("Server Started");
}

void loop() {

  server.handleClient();

  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
  }
  waterRaw = analogRead(SOIL_PIN);

  waterPercent = map(waterRaw, 1023, 300, 0, 100);
  waterPercent = constrain(waterPercent, 0, 100);

  ldrState = digitalRead(LDR_PIN);
  ldrStatus = (ldrState == 0) ? "Bright" : "Dim";

  if (waterPercent < 35 || temperature > 40) {
    plantStatus = "STRESS";
  } else {
    plantStatus = "NORMAL";
  }

if (waterPercent < 35) {
  digitalWrite(RELAY_PIN, HIGH);
  pumpStatus = "ON";
} else {
  digitalWrite(RELAY_PIN, LOW);
  pumpStatus = "OFF";
}

  lcd.setCursor(0, 0);
  lcd.print("Tmp:");
  lcd.print(temperature, 1);
  lcd.print(" Hm:");
  lcd.print(humidity, 1);
  lcd.print(" ");

  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(pumpStatus);
  lcd.print(" W:");
  lcd.print(waterPercent);
  lcd.print("%   ");
  //delay(500);

  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 15000) {  // every 15 sec
    lastUpdate = millis();
    if (WiFi.status() == WL_CONNECTED) {
  WiFiClient client;
  HTTPClient http;

  String url = "http://api.thingspeak.com/update?api_key=KC6WQRM813K0VBLH";
  url += "&field1=" + String(temperature);
  url += "&field2=" + String(humidity);
  url += "&field3=" + String(waterPercent);
  url += "&field4=" + String(ldrState);

  http.begin(client, url);   
  int httpCode = http.GET();

  Serial.print("ThingSpeak Response: ");
  Serial.println(httpCode);

  http.end();
}
  }
}

void handleRoot() {

String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>Plant Monitoring Dashboard</title>

<style>

body{
margin:0;
font-family:'Segoe UI',sans-serif;
background:linear-gradient(135deg,#4ade80,#22c55e);
display:flex;
justify-content:center;
align-items:flex-start;
min-height:100vh;
padding:20px;
transition:0.3s;
}

.dark{
background:#0f172a;
color:white;
}

.container{
width:95%;
max-width:1000px;
background:white;
padding:20px;
border-radius:20px;
box-shadow:0 15px 40px rgba(0,0,0,0.2);
transition:0.3s;
}

.dark .container{
background:#1e293b;
}

.title{
text-align:center;
font-size:22px;
font-weight:bold;
margin-bottom:15px;
}

.grid{
display:grid;
grid-template-columns:repeat(2,1fr);
gap:12px;
}

.card{
background:#f0fdf4;
padding:15px;
border-radius:12px;
text-align:center;
}

.dark .card{
background:#334155;
}

.value{
font-size:22px;
font-weight:bold;
}

/* 🔔 Notification Banner */
.banner{
display:none;
margin-bottom:10px;
padding:10px;
border-radius:10px;
text-align:center;
font-weight:bold;
}

.banner.stress{
display:block;
background:#fecaca;
color:#7f1d1d;
}

.banner.normal{
display:block;
background:#bbf7d0;
color:#065f46;
}

.status{
margin-top:15px;
padding:12px;
text-align:center;
border-radius:10px;
font-weight:bold;
}

.NORMAL{background:#bbf7d0;}
.STRESS{background:#fecaca;}

.tabs{
margin-top:15px;
display:flex;
gap:10px;
justify-content:center;
}

.tab{
padding:8px 12px;
background:#e2e8f0;
border-radius:8px;
cursor:pointer;
}

.graph{
margin-top:15px;
}

iframe{
width:100%;
height:250px;
border:none;
border-radius:10px;
}

.toggle{
position:absolute;
top:20px;
right:20px;
cursor:pointer;
}

</style>
</head>

<body>

<div class="toggle" onclick="toggleDark()">🌙</div>

<div class="container">

<div class="title">
🌿 Renewable Energy Powered IoT System for Plant Water Stress Monitoring
</div>

<!-- 🔔 Notification Banner -->
<div id="banner" class="banner normal">
System is Normal 🌿
</div>

<div class="grid">

<div class="card">
<div>🌡 Temp</div>
<div class="value" id="temp">--</div>
</div>

<div class="card">
<div>💧 Humidity</div>
<div class="value" id="humi">--</div>
</div>

<div class="card">
<div>🌱 Plant Water</div>
<div class="value" id="soil">--</div>
</div>

<div class="card">
<div>☀ Light</div>
<div class="value" id="ldr">--</div>
</div>

<div class="card">
<div>🚰 Pump</div>
<div class="value" id="pump">--</div>
</div>

</div>

<div id="statusBox" class="status NORMAL">
🌿 Status: <span id="status">Loading</span>
</div>

<!-- 📊 Graph Tabs -->
<div class="tabs">
<div class="tab" onclick="loadGraph(1)">Temp</div>
<div class="tab" onclick="loadGraph(2)">Humidity</div>
<div class="tab" onclick="loadGraph(3)">Water</div>
<div class="tab" onclick="loadGraph(4)">Light</div>
</div>

<div class="graph">
<iframe id="graphFrame"
src="https://thingspeak.com/channels/3325561/charts/3"></iframe>
</div>

</div>

<script>

function update(){
fetch('/data')
.then(res=>res.json())
.then(data=>{

document.getElementById("temp").innerText=data.temp+"°C";
document.getElementById("humi").innerText=data.humi+"%";
document.getElementById("soil").innerText=data.soil+"%";
document.getElementById("ldr").innerText=data.ldr;
document.getElementById("pump").innerText=data.pump;

document.getElementById("status").innerText=data.status;

let box=document.getElementById("statusBox");
box.className="status "+data.status;

/* 🔔 Banner Logic */
let banner = document.getElementById("banner");

if(data.status === "STRESS"){
banner.className = "banner stress";
banner.innerText = "⚠️ Plant is in STRESS!";
} else {
banner.className = "banner normal";
banner.innerText = "🌿 System is Normal";
}

});
}

function loadGraph(field){
document.getElementById("graphFrame").src =
"https://thingspeak.com/channels/3325561/charts/"+field;
}

function toggleDark(){
document.body.classList.toggle("dark");
}

setInterval(update,2000);
update();

</script>

</body>
</html>
)rawliteral";

server.send(200, "text/html", page);
}

void handleData() {
  String json = "{";
  json += "\"temp\":\"" + String(temperature, 1) + "\",";
  json += "\"humi\":\"" + String(humidity) + "\",";
  json += "\"soil\":\"" + String(waterPercent) + "\",";
  json += "\"ldr\":\"" + ldrStatus + "\",";
  json += "\"status\":\"" + plantStatus + "\",";
  json += "\"pump\":\"" + pumpStatus + "\"";
  json += "}";
  server.send(200, "application/json", json);
}
