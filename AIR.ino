#include <DHT.h>  // Including library for dht
#include "MQ135.h"
#include "MQ2.h"
#include "MQ7.h"
#include "MQ8.h"
#include "MQ4.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Pushbullet settings
const char* PUSHBULLET_API_KEY = "";  // Your Pushbullet API key
const char* PUSHBULLET_DEVICE_IDEN = "";  // Your device identifier

// Gas threshold values (in PPM)
const int CO2_THRESHOLD = 300;      // CO2 threshold
const int LPG_THRESHOLD = 100;       // LPG/Propane threshold
const int CO_THRESHOLD = 50;         // Carbon Monoxide threshold
const int H2_THRESHOLD = 100;        // Hydrogen threshold
const int CH4_THRESHOLD = 100;       // Natural Gas threshold

String apiKey = "";     //  Enter your Write API key from ThingSpeak
 
const char *ssid =  "";     // replace with your wifi ssid and wpa2 key
const char *pass =  "";
const char* server = "api.thingspeak.com";

// Pin Definitions for NodeMCU
#define MQ135_PIN A0    
#define MQ2_PIN D1      // GPIO5
#define MQ7_PIN D6      // GPIO12 
#define MQ8_PIN D3      // GPIO0
#define MQ4_PIN D4      // GPIO2
#define DHTPIN D2       // GPIO4 

// Global variables for sensor readings
int air_quality;
int mq2_value;
int mq7_value;
int mq8_value;
int mq4_value;
float humidity;
float temperature;

// Timing variables
unsigned long lastThingSpeakUpdate = 0;
unsigned long lastAlertTime = 0;
const unsigned long THINGSPEAK_INTERVAL = 15000;  // 15 seconds between ThingSpeak updates
const unsigned long ALERT_COOLDOWN = 300000;      // 5 minutes between alerts
bool alertSent = false;

// Initialize sensors as global objects
DHT dht(DHTPIN, DHT11);
MQ135 gasSensor(MQ135_PIN);
MQ2 mq2Sensor(MQ2_PIN);
MQ7 mq7Sensor(MQ7_PIN);
MQ8 mq8Sensor(MQ8_PIN);
MQ4 mq4Sensor(MQ4_PIN);
 
WiFiClient client;
WiFiClientSecure secureClient;

// Function to escape special characters in JSON
String escapeJson(const String& str) {
    String escaped = "";
    for (unsigned int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (c == '\n') {
            escaped += "\\n";
        } else if (c == '\r') {
            escaped += "\\r";
        } else if (c == '\t') {
            escaped += "\\t";
        } else if (c == '"') {
            escaped += "\\\"";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else {
            escaped += c;
        }
    }
    return escaped;
}

// Function to send Pushbullet notification
bool sendPushbulletAlert(String title, String body) {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[DEBUG] Attempting to send Pushbullet notification...");
        Serial.println("[DEBUG] Title: " + title);
        Serial.println("[DEBUG] Body: " + body);
        
        HTTPClient http;
        String url = "https://api.pushbullet.com/v2/pushes";
        Serial.println("[DEBUG] URL: " + url);
        
        // Skip SSL certificate verification
        secureClient.setInsecure();
        
        http.begin(secureClient, url);
        http.addHeader("Access-Token", PUSHBULLET_API_KEY);
        http.addHeader("Content-Type", "application/json");
        
        // Create JSON payload with escaped characters
        String jsonPayload = "{\"device_iden\":\"" + String(PUSHBULLET_DEVICE_IDEN) + 
                           "\",\"type\":\"note\",\"title\":\"" + escapeJson(title) + 
                           "\",\"body\":\"" + escapeJson(body) + "\"}";
        
        Serial.println("[DEBUG] JSON Payload: " + jsonPayload);
        
        // Add retry mechanism
        int maxRetries = 3;
        int retryCount = 0;
        int httpResponseCode = -1;
        
        while (retryCount < maxRetries) {
            httpResponseCode = http.POST(jsonPayload);
            Serial.print("[DEBUG] HTTP Response code (attempt " + String(retryCount + 1) + "): ");
            Serial.println(httpResponseCode);
            
            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println("[DEBUG] Response: " + response);
                http.end();
                return (httpResponseCode == 200);
            } else {
                Serial.print("[DEBUG] Error code: ");
                Serial.println(httpResponseCode);
                Serial.println("[DEBUG] Error: " + http.errorToString(httpResponseCode));
                retryCount++;
                if (retryCount < maxRetries) {
                    Serial.println("[DEBUG] Retrying in 2 seconds...");
                    delay(2000);
                }
            }
        }
        
        http.end();
        return false;
    } else {
        Serial.println("[DEBUG] WiFi not connected. Cannot send notification.");
        Serial.print("[DEBUG] WiFi status: ");
        Serial.println(WiFi.status());
        return false;
    }
}

// Function to update ThingSpeak
void updateThingSpeak() {
    Serial.println("\n[DEBUG] Attempting to update ThingSpeak...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[DEBUG] WiFi not connected. Cannot update ThingSpeak.");
        return;
    }
    
    Serial.println("[DEBUG] Connecting to ThingSpeak server...");
    if (client.connect(server, 80)) {
        Serial.println("[DEBUG] Connected to ThingSpeak server");
        
        String postStr = apiKey;
        postStr += "&field1=" + String(temperature);
        postStr += "&field2=" + String(humidity);
        postStr += "&field3=" + String(air_quality);
        postStr += "&field4=" + String(mq2_value);
        postStr += "&field5=" + String(mq7_value);
        postStr += "&field6=" + String(mq8_value);
        postStr += "&field7=" + String(mq4_value);
        postStr += "\r\n\r\n";

        Serial.println("[DEBUG] Sending data to ThingSpeak:");
        Serial.println("[DEBUG] Temperature: " + String(temperature));
        Serial.println("[DEBUG] Humidity: " + String(humidity));
        Serial.println("[DEBUG] CO2: " + String(air_quality));
        Serial.println("[DEBUG] LPG: " + String(mq2_value));
        Serial.println("[DEBUG] CO: " + String(mq7_value));
        Serial.println("[DEBUG] H2: " + String(mq8_value));
        Serial.println("[DEBUG] CH4: " + String(mq4_value));

        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);

        // Wait for server response
        delay(1000);
        
        // Check if there's a response
        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") {
                Serial.println("[DEBUG] Headers received");
                break;
            }
            Serial.println("[DEBUG] Response: " + line);
        }

        Serial.println("[DEBUG] Data sent to ThingSpeak successfully");
    } else {
        Serial.println("[DEBUG] Connection to ThingSpeak server failed");
    }
    
    client.stop();
    Serial.println("[DEBUG] ThingSpeak connection closed");
}

// Function to check gas levels and send alerts
void checkGasLevels() {
    String alertMessage = "";
    bool needAlert = false;
    
    Serial.println("\n[DEBUG] Checking gas levels against thresholds:");
    Serial.println("[DEBUG] CO2: " + String(air_quality) + " PPM (Threshold: " + String(CO2_THRESHOLD) + ")");
    Serial.println("[DEBUG] LPG: " + String(mq2_value) + " PPM (Threshold: " + String(LPG_THRESHOLD) + ")");
    Serial.println("[DEBUG] CO: " + String(mq7_value) + " PPM (Threshold: " + String(CO_THRESHOLD) + ")");
    Serial.println("[DEBUG] H2: " + String(mq8_value) + " PPM (Threshold: " + String(H2_THRESHOLD) + ")");
    Serial.println("[DEBUG] CH4: " + String(mq4_value) + " PPM (Threshold: " + String(CH4_THRESHOLD) + ")");
    
    if (air_quality > CO2_THRESHOLD) {
        alertMessage += "CO2 level high: " + String(air_quality) + " PPM";
        needAlert = true;
        Serial.println("[DEBUG] CO2 threshold exceeded!");
    }
    if (mq2_value > LPG_THRESHOLD) {
        alertMessage += "LPG/Propane level high: " + String(mq2_value) + " PPM";
        needAlert = true;
        Serial.println("[DEBUG] LPG threshold exceeded!");
    }
    if (mq7_value > CO_THRESHOLD) {
        alertMessage += "Carbon Monoxide level high: " + String(mq7_value) + " PPM";
        needAlert = true;
        Serial.println("[DEBUG] CO threshold exceeded!");
    }
    if (mq8_value > H2_THRESHOLD) {
        alertMessage += "Hydrogen level high: " + String(mq8_value) + " PPM";
        needAlert = true;
        Serial.println("[DEBUG] H2 threshold exceeded!");
    }
    if (mq4_value > CH4_THRESHOLD) {
        alertMessage += "Natural Gas level high: " + String(mq4_value) + " PPM";
        needAlert = true;
        Serial.println("[DEBUG] CH4 threshold exceeded!");
    }
    
    unsigned long currentTime = millis();
    Serial.println("[DEBUG] Current time: " + String(currentTime));
    Serial.println("[DEBUG] Last alert time: " + String(lastAlertTime));
    Serial.println("[DEBUG] Time since last alert: " + String(currentTime - lastAlertTime));
    Serial.println("[DEBUG] Alert cooldown: " + String(ALERT_COOLDOWN));
    Serial.println("[DEBUG] Alert sent flag: " + String(alertSent));
    
    if (needAlert && (!alertSent || (currentTime - lastAlertTime > ALERT_COOLDOWN))) {
        Serial.println("[DEBUG] Sending alert! Need alert: " + String(needAlert));
        String title = "⚠️ POLLUTION ALERT ⚠️";
        alertMessage = "Gas levels have exceeded safety thresholds:\n" + alertMessage;
        if (sendPushbulletAlert(title, alertMessage)) {
            alertSent = true;
            lastAlertTime = currentTime;
            Serial.println("[DEBUG] Alert sent successfully");
        } else {
            Serial.println("[DEBUG] Failed to send alert");
            alertSent = false;  // Reset flag to try again next time
        }
    } else {
        Serial.println("[DEBUG] Not sending alert. Need alert: " + String(needAlert));
        if (alertSent) {
            Serial.println("[DEBUG] Alert already sent");
        }
        if (currentTime - lastAlertTime <= ALERT_COOLDOWN) {
            Serial.println("[DEBUG] Still in cooldown period");
        }
    }
    
    if (!needAlert) {
        alertSent = false;
        Serial.println("[DEBUG] Reset alert sent flag - all levels normal");
    }
}

// Function to read all sensor values
void readSensors() {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    air_quality = gasSensor.getPPM();
    mq2_value = mq2Sensor.getPPM();
    mq7_value = mq7Sensor.getPPM();
    mq8_value = mq8Sensor.getPPM();
    mq4_value = mq4Sensor.getPPM();
    
    // Print sensor readings
    Serial.println("\n[DEBUG] Sensor Readings:");
    Serial.println("[DEBUG] Temperature: " + String(temperature) + "°C");
    Serial.println("[DEBUG] Humidity: " + String(humidity) + "%");
    Serial.println("[DEBUG] CO2: " + String(air_quality) + " PPM");
    Serial.println("[DEBUG] LPG: " + String(mq2_value) + " PPM");
    Serial.println("[DEBUG] CO: " + String(mq7_value) + " PPM");
    Serial.println("[DEBUG] H2: " + String(mq8_value) + " PPM");
    Serial.println("[DEBUG] CH4: " + String(mq4_value) + " PPM");
}
 
void setup() {
    Serial.begin(115200);
    delay(10);
    
    dht.begin();
    
    Serial.println("[DEBUG] Connecting to WiFi...");
    Serial.println("[DEBUG] SSID: " + String(ssid));
    
    WiFi.begin(ssid, pass);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("[DEBUG] WiFi connected");
    Serial.print("[DEBUG] IP address: ");
    Serial.println(WiFi.localIP());
    
    // Send a test notification
    Serial.println("[DEBUG] Sending test notification...");
    if (sendPushbulletAlert("Air Quality Monitor Started", "The device is now online and monitoring air quality.")) {
        Serial.println("[DEBUG] Test notification sent successfully");
    } else {
        Serial.println("[DEBUG] Failed to send test notification");
    }
}
 
void loop() {
    unsigned long currentTime = millis();
    
    // Read sensors
    readSensors();
    
    // Check gas levels and send alerts if needed
    checkGasLevels();
    
    // Update ThingSpeak every 15 seconds
    if (currentTime - lastThingSpeakUpdate >= THINGSPEAK_INTERVAL) {
        Serial.println("\n[DEBUG] Time to update ThingSpeak");
        Serial.println("[DEBUG] Last update: " + String(lastThingSpeakUpdate));
        Serial.println("[DEBUG] Current time: " + String(currentTime));
        Serial.println("[DEBUG] Time since last update: " + String(currentTime - lastThingSpeakUpdate));
        
        updateThingSpeak();
        lastThingSpeakUpdate = currentTime;
    }
    
    // Small delay to prevent overwhelming the system
    delay(10000);
}
