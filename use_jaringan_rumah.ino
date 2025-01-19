#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Wire.h>

// Wi-Fi credentials
const char* ssid = "GEORGIA";  // Replace with your Wi-Fi SSID
const char* password = "Georgia12345";  // Replace with your Wi-Fi password

// Wi-Fi access point credentials (fallback mode)
const char* ap_ssid = "tesip";
const char* ap_password = "12345678";

// Static IP configuration for Wi-Fi connection
IPAddress local_IP(192, 168, 1, 5); // Set the static IP address
IPAddress gateway(192, 168, 1, 1);  // Set the gateway (usually your router's IP)
IPAddress subnet(255, 255, 255, 0); // Set the subnet mask

// GPIO pins for relays (active low)
const int relayPins[] = {5, 4, 0, 2};

// Relay states
bool relayStates[] = {HIGH, HIGH, HIGH, HIGH}; // Start with relays off
bool autoControlRelay4 = false; // Automatic control state for relay 4

// DHT22 sensor setup
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;

// Create a WebServer object on port 80
ESP8266WebServer server(80);

// Toggle relay function
void toggleRelay(int relayIndex) {
  relayStates[relayIndex] = !relayStates[relayIndex];
  digitalWrite(relayPins[relayIndex], relayStates[relayIndex]);
  Serial.print("Relay ");
  Serial.print(relayIndex + 1);
  Serial.println(relayStates[relayIndex] == LOW ? " ON" : " OFF");
}

// Automatic control for relay 4 based on temperature
void autoControlRelay() {
  if (autoControlRelay4) {
    if (temperature <= 24.4) {
      relayStates[3] = HIGH; // Turn OFF relay 4
    } else if (temperature >= 29.0) {
      relayStates[3] = LOW; // Turn ON relay 4
    }
    digitalWrite(relayPins[3], relayStates[3]);
  }
}

// HTML content generation function with modular classes and colors
String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css\">"
                "<style>body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #1C1C2D; color: #FFFFFF; }"
                ".container { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; padding: 20px; }"
                ".card { background: linear-gradient(145deg, #2C2C3E, #232330); box-shadow: 5px 5px 15px #18181f, -5px -5px 15px #2e2e3d; border-radius: 20px; padding: 20px; text-align: center; }"
                ".card h2 { margin: 10px 0; font-size: 18px; }"
                ".card p { font-size: 16px; margin: 5px 0; }"
                ".temperature { background: linear-gradient(145deg, #ff7eb3, #d65d91); }"
                ".humidity { background: linear-gradient(145deg, #5dc8ff, #478ec6); }"
                ".relay { background: linear-gradient(145deg, #69e7bd, #4cb896); }"
                ".button { display: block; padding: 10px 15px; margin-top: 10px; font-size: 14px; color: #FFFFFF; background: linear-gradient(145deg, #3B3B4F, #2A2A38); border: none; border-radius: 10px; cursor: pointer; }"
                ".button.off { background: linear-gradient(145deg, #ff5e5e, #cc4848); }"
                ".icon { font-size: 24px; margin-bottom: 10px; color: #FFF; }"
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
                "setInterval(fetchData, 2000); // Refresh every 2 seconds"
                "</script></head><body>";

  html += "<h1 style=\"text-align: center; margin-top: 20px;\">ESP8266 Sensor Dashboard</h1>";
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
  html += "<div class=\"card relay\">";
  html += "<i class=\"fas fa-cogs icon\"></i>";
  html += "<h2>Relay 4 Automatic Control</h2>";
  html += "<p>Status: " + String(autoControlRelay4 ? "Enabled" : "Disabled") + "</p>";
  html += "<form action=\"/toggle_auto_4\" method=\"GET\">";
  html += "<button class=\"button " + String(autoControlRelay4 ? "off" : "") + "\" type=\"submit\">"
          + (autoControlRelay4 ? "Disable" : "Enable") + "</button></form>";
  html += "</div>";

  html += "</div></body></html>";
  return html;
}

// Read DHT sensor data
void readDHTSensor() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    temperature = 0.0;
    humidity = 0.0;
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C");
    Serial.print(" Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
}

// Setup server routes
void setupServerRoutes() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });

  server.on("/get_sensor_data", HTTP_GET, []() {
    String json = "{\"temperature\": " + String(temperature, 1) + ", \"humidity\": " + String(humidity, 1) + "}";
    server.send(200, "application/json", json);
  });

  for (int i = 0; i < 4; i++) {
    int relayIndex = i;
    server.on(("/toggle_relay_" + String(i)).c_str(), HTTP_GET, [relayIndex]() {
      toggleRelay(relayIndex);
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
    digitalWrite(relayPins[i], HIGH); // Relays initially off
  }

  // Initialize DHT sensor
  dht.begin();

  // Set static IP configuration
  WiFi.config(local_IP, gateway, subnet);

  // Connect to Wi-Fi network
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
    Serial.println(WiFi.localIP());  // Print the IP address assigned by your home Wi-Fi
  } else {
    Serial.println("Failed to connect to Wi-Fi, switching to AP mode.");

    // If failed to connect, create Access Point
    WiFi.softAPConfig(local_IP, gateway, subnet);
    if (WiFi.softAP(ap_ssid, ap_password, 6)) {
      Serial.println("Access Point created successfully.");
    } else {
      Serial.println("Failed to create Access Point.");
    }

    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());  // Print the IP address for Access Point mode
  }

  setupServerRoutes();
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  readDHTSensor(); // Read temperature and humidity
  autoControlRelay(); // Automatic control for relay 4
  server.handleClient();
}
