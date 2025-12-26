#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
// --- WIFI SETTINGS ---
const char* ssid = "Noor";         // CHANGE THIS
const char* password = "noor2005"; // CHANGE THIS

AsyncWebServer server(80);
String latestSensorJson = "{\"presence\":\"--\",\"temp\":0,\"humidity\":0,\"fire\":\"--\",\"water\":0,\"dayNight\":\"--\"}";


void setup() {
  Serial.begin(115200);
  
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  
  // --- IMPORTANT: Set ESP32 to receive at 9600 to match Uno ---
  Serial2.begin(9600, SERIAL_8N1, 17, 16);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", latestSensorJson);
  });

  server.on("/servo", HTTP_POST, [](AsyncWebServerRequest *request){
    String target = request->hasParam("target", true) ? request->getParam("target", true)->value() : "";
    String action = request->hasParam("action", true) ? request->getParam("action", true)->value() : "";
    int angle = (action == "open") ? 90 : 0;
    int index = (target == "window1") ? 1 : (target == "window2") ? 2 : (target == "door") ? 3 : 0;
    
    if (index > 0) Serial2.printf("SRV:%d:%d\n", index, angle);
    request->send(200, "text/plain", "OK");
  });

  server.on("/fan", HTTP_POST, [](AsyncWebServerRequest *request){
    String action = request->hasParam("action", true) ? request->getParam("action", true)->value() : "off";
    Serial2.printf("FAN:0:%d\n", (action == "on") ? 1 : 0);
    request->send(200, "text/plain", "OK");
  });

  server.on("/light", HTTP_POST, [](AsyncWebServerRequest *request){
    String light = request->hasParam("light", true) ? request->getParam("light", true)->value() : "";
    String val = request->hasParam("value", true) ? request->getParam("value", true)->value() : "0";
    int index = (light == "light1") ? 1 : (light == "light2") ? 2 : 0;
    
    if (index > 0) Serial2.printf("LGT:%d:%s\n", index, val.c_str());
    request->send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  if (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    incoming.trim();
    int start = incoming.indexOf('{');
    int end = incoming.lastIndexOf('}');
    if (start != -1 && end != -1 && end > start) {
       latestSensorJson = incoming.substring(start, end + 1);
       Serial.println("WEBSITE UPDATED: " + latestSensorJson);
    }
  }
}