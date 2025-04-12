#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "GEORGIA";  // Nama WiFi rumah
const char* password = "Georgia12345";  // Password WiFi rumah
const char* ap_ssid = "tesip";  // Nama SSID untuk Access Point
const char* ap_password = "12345678";  // Password untuk Access Point

// IP statis untuk Access Point
IPAddress local_IP(192, 168, 1, 5);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// Pin untuk relay (D1, D2, D3, D4)
const int relayPins[] = {5, 4, 0, 2};  // D1, D2, D3, D4
bool relayStates[] = {HIGH, HIGH, HIGH, HIGH};  // Status relay (HIGH = OFF, LOW = ON)
bool autoControlRelay4 = false;

#define DHTPIN 15  // Pin untuk sensor DHT22
#define DHTTYPE DHT22  // Tipe sensor DHT
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

ESP8266WebServer server(80);  // Web server berjalan di port 80

// Fungsi untuk menyalakan atau mematikan relay
void toggleRelay(int relayIndex) {
  relayStates[relayIndex] = !relayStates[relayIndex];
  digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
}

// Fungsi untuk mengontrol relay otomatis berdasarkan suhu
void autoControlRelay() {
  if (autoControlRelay4) {
    if (temperature <= 24.4) {
      relayStates[3] = HIGH;  // Mematikan relay 4
    } else if (temperature >= 29.0) {
      relayStates[3] = LOW;  // Menyalakan relay 4
    }
    digitalWrite(relayPins[3], relayStates[3]);
  }
}

// Fungsi untuk membaca data dari sensor DHT
void readDHTSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    temperature = 0.0;
    humidity = 0.0;
  }
}

// Fungsi untuk menghasilkan HTML yang ditampilkan di browser
String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
                "<style>"
                "body{font-family:Arial;background:#1E1E2F;color:#fff;padding:10px;text-align:center;transition:0.5s}"
                ".light{background:#f2f2f2;color:#111}"
                ".card{background:#2E2E3E;margin:10px;padding:10px;border-radius:10px;box-shadow:0 2px 10px #000;transition:0.5s}"
                ".light .card{background:#fff;box-shadow:0 2px 10px #ccc}"
                "button{padding:10px 20px;margin:5px;border:none;border-radius:5px;cursor:pointer;background:#4CAF50;color:white}"
                "button.off{background:#F44336}"
                "button.mode{background:#2196F3}"
                "</style>"
                "<script>"
                "let isLight=false;"
                "function fetchData(){fetch('/status').then(r=>r.json()).then(d=>{"
                "document.getElementById('temperature').innerHTML=d.temperature+' °C';"
                "document.getElementById('humidity').innerHTML=d.humidity+' %';"
                "for(let i=0;i<4;i++){"
                "let btn=document.getElementById('relay'+i);"
                "btn.innerHTML=d.relays[i]==0?'Turn OFF':'Turn ON';"
                "btn.className=d.relays[i]==0?'off':'';"
                "document.getElementById('relay_status'+i).innerHTML=d.relays[i]==0?'ON':'OFF';}})}"
                "setInterval(fetchData,2000);"
                "function toggleRelay(i){"
                "let btn=document.getElementById('relay'+i);"
                "btn.innerHTML='...';"
                "fetch('/toggle_relay_'+i).then(()=>fetchData());"
                "}"
                "function toggleAuto(){fetch('/toggle_auto_4').then(()=>fetchData());}"
                "function toggleMode(){"
                "isLight=!isLight;"
                "document.body.className=isLight?'light':'';"
                "document.getElementById('modeBtn').innerHTML=isLight?'Dark Mode':'Light Mode';"
                "}"
                "</script></head><body>"
                "<h2>ESP8266 Dashboard</h2>"
                "<button class='mode' id='modeBtn' onclick='toggleMode()'>Light Mode</button>";

  html += "<div class='card'><h3>Temperature</h3><p id='temperature'>Loading...</p></div>";
  html += "<div class='card'><h3>Humidity</h3><p id='humidity'>Loading...</p></div>";

  for (int i = 0; i < 4; i++) {
    html += "<div class='card'><h3>Relay " + String(i + 1) + "</h3><p id='relay_status" + i + "'>OFF</p>"
            "<button id='relay" + i + "' onclick='toggleRelay(" + i + ")'>Loading...</button></div>";
  }

  html += "<div class='card'><h3>Auto Relay 4</h3><p>Status: " + String(autoControlRelay4 ? "Enabled" : "Disabled") + "</p>"
          "<button onclick='toggleAuto()'>" + String(autoControlRelay4 ? "Disable" : "Enable") + "</button></div>";

  html += "</body></html>";
  return html;
}

// Fungsi untuk mengatur route server
void setupServerRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });

  server.on("/status", HTTP_GET, []() {
    StaticJsonDocument<256> json;
    json["temperature"] = temperature;
    json["humidity"] = humidity;
    JsonArray relays = json.createNestedArray("relays");
    for (int i = 0; i < 4; i++) relays.add(relayStates[i]);
    String output;
    serializeJson(json, output);
    server.send(200, "application/json", output);
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
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], relayStates[i]);
  }

  dht.begin();

  // Mengatur mode WiFi menjadi dual mode: STA + AP
  WiFi.mode(WIFI_AP_STA);  
  WiFi.config(local_IP, gateway, subnet);  // Atur IP statis untuk AP jika perlu
  WiFi.begin(ssid, password);  // Terhubung ke WiFi rumah
  
  // Jika gagal terhubung, akan mencoba kembali
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
    Serial.print("STA IP: "); 
    Serial.println(WiFi.localIP());  // IP yang didapat dari WiFi rumah
  } else {
    Serial.println("Failed to connect to WiFi");
  }

  // Mengaktifkan AP dengan SSID 'tesip'
  WiFi.softAP(ap_ssid, ap_password); 
  Serial.print("AP IP: "); 
  Serial.println(WiFi.softAPIP());  // IP dari Access Point
  
  setupServerRoutes();
  server.begin();
}

void loop() {
  readDHTSensor();
  autoControlRelay();
  server.handleClient();
}
