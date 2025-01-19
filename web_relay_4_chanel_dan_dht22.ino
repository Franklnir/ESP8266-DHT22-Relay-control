#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <Wire.h>

// Wi-Fi access point credentials
const char* ssid = "tesip";
const char* password = "12345678";

// Static IP configuration
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// GPIO pins for relays (active low)
const int relayPins[] = {5, 4, 0, 2};

// Relay states
bool relayStates[] = {HIGH, HIGH, HIGH, HIGH}; // Start with relays off

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

// HTML content generation function with modular classes and colors
String generateHTML() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                "<style>body { font-family: Arial, sans-serif; max-width: 400px; margin: auto; text-align: center; background: #f3f4f6; padding: 20px; }"
                ".card { margin: 15px; padding: 20px; border-radius: 10px; color: white; }"
                ".temperature { background: #8E44AD; }"
                ".humidity { background: #3498DB; }"
                ".relay { background: #1ABC9C; }"
                ".button { display: inline-block; width: 80%; padding: 15px; margin: 10px; font-size: 18px; color: white; background-color: #2C3E50; border: none; border-radius: 5px; text-decoration: none; }"
                ".button.off { background-color: #E74C3C; }"
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

  html += "<h1>ESP8266 Sensor Dashboard</h1>";

  // Temperature card
  html += "<div class=\"card temperature\">"
          "<h2>Temperature</h2>"
          "<p id=\"temperature\">" + String(temperature, 1) + " &#8451;</p>"
          "</div>";

  // Humidity card
  html += "<div class=\"card humidity\">"
          "<h2>Humidity</h2>"
          "<p id=\"humidity\">" + String(humidity, 1) + " %</p>"
          "</div>";

  // Relay control cards
  for (int i = 0; i < 4; i++) {
    html += "<div class=\"card relay\">"
            "<h2>Relay " + String(i + 1) + "</h2>"
            "<p>Status: " + (relayStates[i] == LOW ? "ON" : "OFF") + "</p>"
            "<form action=\"/toggle_relay_" + String(i) + "\" method=\"GET\">"
            "<button class=\"button " + String(relayStates[i] == LOW ? "off" : "") + "\" type=\"submit\">" 
            + String(relayStates[i] == LOW ? "Turn OFF" : "Turn ON") + "</button></form>"
            "</div>";
  }

  html += "</body></html>";
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

  // Configure Wi-Fi with a fixed IP and set up access point
  WiFi.softAPConfig(local_IP, gateway, subnet);
  if (WiFi.softAP(ssid, password, 6)) {
    Serial.println("Access Point created successfully.");
  } else {
    Serial.println("Failed to create Access Point.");
  }

  // Check if the AP is working and print the IP address
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  setupServerRoutes();
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  readDHTSensor(); // Read temperature and humidity
  server.handleClient();
}
