#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Wire.h>

// Wi-Fi credentials
const char* ssid = "GEORGIA";  // Replace with your Wi-Fi SSID
const char* password = "Georgia12345";  // Replace with your Wi-Fi password

// Wi-Fi access point credentials
const char* ap_ssid = "tesip";  
const char* ap_password = "12345678";

// Static IP configuration for Wi-Fi connection
IPAddress local_IP(192, 168, 1, 5);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// GPIO pins for relays (active low)
const int relayPins[] = {5, 4, 0, 2};

// Relay states
bool relayStates[] = {HIGH, HIGH, HIGH, HIGH};
bool autoControlRelay4 = false;

// DHT22 sensor setup
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

// Create a WebServer object on port 80
ESP8266WebServer server(80);

// Function to toggle relay
void toggleRelay(int relayIndex) {
  relayStates[relayIndex] = !relayStates[relayIndex];
  digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
}

// Automatic control for relay 4
void autoControlRelay() {
  if (autoControlRelay4) {
    if (temperature <= 24.4) {
      relayStates[3] = HIGH;
    } else if (temperature >= 29.0) {
      relayStates[3] = LOW;
    }
    digitalWrite(relayPins[3], relayStates[3]);
  }
}
String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css\">"
                "<style>"
                "body { font-family: 'Arial', sans-serif; margin: 0; padding: 0; background: linear-gradient(180deg, #1E1E2F, #141423); color: #FFFFFF; }"
                "h1 { text-align: center; margin: 10px 0; font-size: 18px; color: #F8F8F8; text-shadow: 1px 1px 5px #000; }"
                ".container { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; padding: 10px; }"
                ".card { background: linear-gradient(145deg, #32324A, #1F1F2F); box-shadow: 3px 3px 10px #12121D, -3px -3px 10px #3A3A50; "
                "border-radius: 10px; padding: 10px; text-align: center; font-size: 14px; transition: transform 0.2s; }"
                ".card:hover { transform: scale(1.05); }"
                ".card h2 { font-size: 14px; margin: 5px 0; color: #F8F8F8; }"
                ".card p { font-size: 12px; margin: 5px 0; color: #A1A1C1; }"
                ".button { display: block; width: 100%; padding: 8px; font-size: 12px; font-weight: bold; margin-top: 5px; "
                "color: #FFFFFF; background: linear-gradient(145deg, #474760, #343448); border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; }"
                ".button:hover { background: linear-gradient(145deg, #57577A, #42425C); }"
                ".button.off { background: linear-gradient(145deg, #FF5E5E, #CC4848); }"
                ".button.off:hover { background: linear-gradient(145deg, #FF7A7A, #D65555); }"
                ".icon { font-size: 20px; margin-bottom: 10px; color: #FFF; text-shadow: 1px 1px 5px #000; }"
                ".temperature { background: linear-gradient(145deg, #FF7EBC, #D65D92); }"
                ".humidity { background: linear-gradient(145deg, #5DC8FF, #478EC6); }"
                ".relay { background: linear-gradient(145deg, #69E7BD, #4CB896); }"
                ".relay:hover { background: linear-gradient(145deg, #7BF4CD, #55CA9F); }"
                "</style>"
                "<script>"
                "function fetchData() {"
                "  fetch('/get_sensor_data')"
                "    .then(response => response.json())"
                "    .then(data => {"
                "      document.getElementById('temperature').innerHTML = data.temperature + ' &#8451;';"
                "      document.getElementById('humidity').innerHTML = data.humidity + ' %';"
                "    });"
                "}"
                "setInterval(fetchData, 2000);"
                "</script></head><body>";

  html += "<h1>ESP8266 Sensor Dashboard</h1>";
  html += "<div class=\"container\">";

  // Temperature card
  html += "<div class=\"card temperature\">"
          "<i class=\"fas fa-thermometer-half icon\"></i>"
          "<h2>Temperature</h2>"
          "<p id=\"temperature\">" + String(temperature, 1) + " &#8451;</p>"
          "</div>";

  // Humidity card
  html += "<div class=\"card humidity\">"
          "<i class=\"fas fa-tint icon\"></i>"
          "<h2>Humidity</h2>"
          "<p id=\"humidity\">" + String(humidity, 1) + " %</p>"
          "</div>";

  // Relay control cards
  for (int i = 0; i < 4; i++) {
    html += "<div class=\"card relay\">"
            "<i class=\"fas fa-plug icon\"></i>"
            "<h2>Relay " + String(i + 1) + "</h2>"
            "<p>Status: " + (relayStates[i] == LOW ? "ON" : "OFF") + "</p>"
            "<form action=\"/toggle_relay_" + String(i) + "\" method=\"GET\">"
            "<button class=\"button " + String(relayStates[i] == LOW ? "off" : "") + "\" type=\"submit\">"
            + String(relayStates[i] == LOW ? "Turn OFF" : "Turn ON") + "</button></form>"
            "</div>";
  }

  // Automatic control button for relay 4
  html += "<div class=\"card relay\">"
          "<i class=\"fas fa-cogs icon\"></i>"
          "<h2>Relay 4 Automatic Control</h2>"
          "<p>Status: " + String(autoControlRelay4 ? "Enabled" : "Disabled") + "</p>"
          "<form action=\"/toggle_auto_4\" method=\"GET\">"
          "<button class=\"button " + String(autoControlRelay4 ? "off" : "") + "\" type=\"submit\">"
          + (autoControlRelay4 ? "Disable" : "Enable") + "</button></form>"
          "</div>";

  html += "</div></body></html>";
  return html;
}



// Read DHT sensor data
void readDHTSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    temperature = 0.0;
    humidity = 0.0;
  }
}



// Setup server routes
void setupServerRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });
  for (int i = 0; i < 4; i++) {
    server.on(("/toggle_relay_" + String(i)).c_str(), HTTP_GET, [i]() {
      toggleRelay(i);
      server.send(200, "text/html", generateHTML());
    });
  }
  server.on("/toggle_auto_4", HTTP_GET, []() {
    autoControlRelay4 = !autoControlRelay4;
    server.send(200, "text/html", generateHTML());
  });
}

void setup() {
  Serial.begin(115200);

  // Initialize relay pins
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);
  }

  // Initialize DHT sensor
  dht.begin();

  // Set static IP
  WiFi.config(local_IP, gateway, subnet);

  // Try connecting to Wi-Fi
  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi connection failed. Starting Access Point mode...");
  }

  // Start Access Point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  setupServerRoutes();
  server.begin();
}

void loop() {
  readDHTSensor();
  autoControlRelay();
  server.handleClient();
}
