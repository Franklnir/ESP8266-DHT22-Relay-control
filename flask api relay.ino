#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid = "Xiaomi 12";
const char* password = "12345678";

// Server Flask API
const char* server = "http://192.168.1.100:5002";  // Ganti dengan IP server Flask

// Pin Relay (D1, D2, D3, D4)
const int relayPins[4] = {5, 4, 0, 2};  // D1 = GPIO5, D2 = GPIO4, D3 = GPIO0, D4 = GPIO2

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Menghubungkan ke WiFi...");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nTerhubung ke WiFi!");
    
    // Set semua relay sebagai output dan matikan (HIGH = OFF)
    for (int i = 0; i < 4; i++) {
        pinMode(relayPins[i], OUTPUT);
        digitalWrite(relayPins[i], HIGH);
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        getRelayStatus();
    } else {
        Serial.println("Terputus dari WiFi, mencoba menyambung ulang...");
        WiFi.begin(ssid, password);
    }
    delay(5000); // Periksa status setiap 5 detik
}

// Mengambil Status Relay dari Server
void getRelayStatus() {
    WiFiClient client;
    HTTPClient http;

    String url = String(server) + "/get_relay_status";
    Serial.print("Menghubungi: ");
    Serial.println(url);

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Data Diterima: " + payload);

        // Parsing JSON
        DynamicJsonDocument doc(512);
        deserializeJson(doc, payload);
        JsonArray relay_status = doc["relay_status"];

        for (int i = 0; i < 4; i++) {
            if (relay_status[i] == "ON") {
                digitalWrite(relayPins[i], LOW);  // Relay ON
            } else {
                digitalWrite(relayPins[i], HIGH); // Relay OFF
            }
        }
    } else {
        Serial.print("Gagal Mengambil Data! Error: ");
        Serial.println(httpCode);
    }
    http.end();
}

// Mengirim Perintah untuk Mengubah Status Relay
void setRelayStatus(int relayIndex, String status) {
    WiFiClient client;
    HTTPClient http;

    String url = String(server) + "/update_relay";
    Serial.print("Mengirim Permintaan ke: ");
    Serial.println(url);

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");

    // JSON data
    String jsonPayload = "{\"relay_index\":" + String(relayIndex) + ", \"relay_status\":\"" + status + "\"}";
    int httpCode = http.POST(jsonPayload);

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Status Relay Berhasil Diubah");
    } else {
        Serial.print("Gagal Mengubah Status! Error: ");
        Serial.println(httpCode);
    }
    http.end();
}
