#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Wi-Fi credentials
const char* ssid = "GEORGIA";
const char* password = "Georgia12345";

// AP fallback
const char* ap_ssid = "tesip";
const char* ap_password = "12345678";

// Static IP
IPAddress local_IP(192, 168, 1, 5);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Relay setup
const int relayPins[] = {5, 4, 0, 2}; // Sesuaikan dengan board layout ESP32-S3
bool relayStates[] = {HIGH, HIGH, HIGH, HIGH};
bool autoControlRelay4 = false;

// BME280
Adafruit_BME280 bme;
float temperature = 0.0, humidity = 0.0, pressure = 0.0, altitude = 0.0;

WebServer server(80);

// Toggle relay
void toggleRelay(int index) {
  relayStates[index] = !relayStates[index];
  digitalWrite(relayPins[index], relayStates[index]);
}

// Auto control relay 4
void autoControlRelay() {
  if (autoControlRelay4) {
    if (temperature <= 24.4) relayStates[3] = HIGH;
    else if (temperature >= 29.0) relayStates[3] = LOW;
    digitalWrite(relayPins[3], relayStates[3]);
  }
}

// Read sensor
void readSensor() {
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0;
  altitude = bme.readAltitude(1013.25);
}

// Generate HTML + JS
String generateHTML() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body { font-family: Arial; background: #1E1E2F; color: white; margin: 0; padding: 10px; }
h1 { text-align: center; }
.card { background: #2a2a40; border-radius: 10px; padding: 10px; margin: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.5); }
.button { padding: 8px; border: none; border-radius: 5px; margin-top: 5px; cursor: pointer; font-weight: bold; }
.on { background: #2ecc71; color: white; }
.off { background: #e74c3c; color: white; }
</style>
</head><body>
<h1>ESP32-S3 Realtime Dashboard</h1>

<div class="card">
  <h2>Temperature: <span id="temp">--</span> &#8451;</h2>
  <h2>Humidity: <span id="hum">--</span> %</h2>
  <h2>Pressure: <span id="pres">--</span> hPa</h2>
  <h2>Altitude: <span id="alt">--</span> m</h2>
</div>

<div id="relays"></div>

<div class="card">
  <h2>Relay 4 Auto: <span id="autoStatus">--</span></h2>
  <button class="button" onclick="toggleAuto()">Toggle Auto</button>
</div>

<script>
function updateData() {
  fetch('/data')
    .then(res => res.json())
    .then(data => {
      document.getElementById('temp').innerHTML = data.temp.toFixed(1);
      document.getElementById('hum').innerHTML = data.hum.toFixed(1);
      document.getElementById('pres').innerHTML = data.pres.toFixed(1);
      document.getElementById('alt').innerHTML = data.alt.toFixed(1);
      document.getElementById('autoStatus').innerHTML = data.auto ? "Enabled" : "Disabled";

      let relaysHTML = "";
      data.relays.forEach((state, i) => {
        relaysHTML += `<div class="card">
          <h2>Relay ${i + 1}: ${state == 0 ? "ON" : "OFF"}</h2>
          <button class="button ${state == 0 ? "off" : "on"}" onclick="toggleRelay(${i})">
            ${state == 0 ? "Turn OFF" : "Turn ON"}
          </button></div>`;
      });
      document.getElementById("relays").innerHTML = relaysHTML;
    });
}

function toggleRelay(index) {
  fetch('/toggle_relay_' + index).then(() => updateData());
}

function toggleAuto() {
  fetch('/toggle_auto_4').then(() => updateData());
}

setInterval(updateData, 2000);
window.onload = updateData;
</script>
</body></html>
)rawliteral";
  return html;
}

// Setup server routes
void setupRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });

  for (int i = 0; i < 4; i++) {
    server.on(("/toggle_relay_" + String(i)).c_str(), HTTP_GET, [i]() {
      toggleRelay(i);
      server.send(200, "text/plain", "OK");
    });
  }

  server.on("/toggle_auto_4", HTTP_GET, []() {
    autoControlRelay4 = !autoControlRelay4;
    server.send(200, "text/plain", "OK");
  });

  server.on("/data", HTTP_GET, []() {
    String json = "{";
    json += "\"temp\":" + String(temperature, 1) + ",";
    json += "\"hum\":" + String(humidity, 1) + ",";
    json += "\"pres\":" + String(pressure, 1) + ",";
    json += "\"alt\":" + String(altitude, 1) + ",";
    json += "\"auto\":" + String(autoControlRelay4 ? "true" : "false") + ",";
    json += "\"relays\":[";
    for (int i = 0; i < 4; i++) {
      json += String(relayStates[i]);
      if (i < 3) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  });
}

void setup() {
  Serial.begin(115200);

  // Set relay pins
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], relayStates[i]);
  }

  // BME280 I2C init
  Wire.begin(8, 9); // ESP32-S3: SDA = GPIO8, SCL = GPIO9
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
    while (1);
  }

  // WiFi connection
  WiFi.begin(ssid, password);
  WiFi.config(local_IP, gateway, subnet);

  Serial.print("Connecting to WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected. IP: " + WiFi.localIP().toString());
  } else {
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("\nAP Mode IP: " + WiFi.softAPIP().toString());
  }

  setupRoutes();
  server.begin();
}

void loop() {
  readSensor();
  autoControlRelay();
  server.handleClient();
}
