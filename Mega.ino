#include <DHT.h>
#include <Servo.h>
#include <SoftwareSerial.h> // Required for Uno to talk to ESP32

// --- PIN DEFINITIONS (UNO MAPPED) ---

// Software Serial for ESP32 Communication
// Connect ESP32 TX to Uno Pin 10
// Connect ESP32 RX to Uno Pin 9
const int rxPin = 10;
const int txPin = 9;
SoftwareSerial espSerial(rxPin, txPin);

// Sensors
const int dhtPin = 12;     
const int trigPin = 7;     // Moved from 5 (to save PWM pin)
const int echoPin = 8;     // Moved from 18 (Pin 18 doesn't exist on Uno)
const int flamePin = A0;   
const int waterPin = A1;   
const int ldrPin = A2;     

// Actuators
const int buzzerPin = 13;  // Moved from 14
const int fanPin = A3;     // Moved from 26 (Using Analog pin as Digital Out)

// Lights (Must be PWM)
// Note: Servo library disables PWM on pins 9 and 10 on Uno
const int light1Pin = 11; 
const int light2Pin = 6;   // Moved from 12 (12 is not PWM on Uno)

// Servos
Servo servo1;
Servo servo2;
Servo servo3;
const int servo1Pin = 2;   
const int servo2Pin = 3;   
const int servo3Pin = 4;   

// Setup Objects
#define DHTTYPE DHT11   
DHT dht(dhtPin, DHTTYPE);

// Variables
unsigned long lastSensorRead = 0;
const long interval = 2000; // Send data every 2 seconds

void setup() {
  // Debug Serial (Computer via USB)
  Serial.begin(9600);
  
  // ESP32 Serial (SoftwareSerial)
  // NOTE: SoftwareSerial is unstable at 115200. 
  // Highly recommended to change ESP32 code to use 9600 baud as well.
  espSerial.begin(9600); 

  // Init Pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(light1Pin, OUTPUT);
  pinMode(light2Pin, OUTPUT);

  // Init Servos
  servo1.attach(servo1Pin);
  servo2.attach(servo2Pin);
  servo3.attach(servo3Pin);
  servo1.write(0);
  servo2.write(0);
  servo3.write(0);

  dht.begin();
  Serial.println("Uno Ready. Waiting for commands...");
}

void loop() {
  // 1. LISTEN FOR COMMANDS FROM ESP32
  if (espSerial.available()) {
    String cmd = espSerial.readStringUntil('\n');
    processCommand(cmd);
  }

  // 2. READ SENSORS AND SEND TO ESP32
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorRead >= interval) {
    lastSensorRead = currentMillis;
    sendSensorData();
  }
}

void sendSensorData() {
  // Ultrasonic
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000); 
  int distance = (duration == 0) ? 999 : duration * 0.034 / 2;
  String presence = (distance < 100 && distance > 0) ? "Someone in room" : "Room empty";

  // DHT
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (isnan(temp)) temp = 0.0;
  if (isnan(hum)) hum = 0.0;

  // Debug to PC
  Serial.print("T:"); Serial.print(temp);
  Serial.print(" H:"); Serial.println(hum);

  // Flame
  int flameVal = analogRead(flamePin);
  // Note: Flame sensors are usually active LOW (Low value = Fire)
  String fire = (flameVal < 500) ? "FLAME DETECTED! WARNING!" : "No flame detected";
  
  // Local logic: Buzzer
  if (flameVal < 500) tone(buzzerPin, 3000);
  else noTone(buzzerPin);

  // Water & LDR
  int waterVal = analogRead(waterPin);
  if (waterVal < 50) waterVal = 0; // Noise filter
  
  int ldrVal = analogRead(ldrPin);
  String dayNight = (ldrVal > 300) ? "Night" : "Morning/Day";

  // Create JSON String manually
  String json = "{\"presence\":\"" + presence + "\"" +
                ",\"temp\":" + String(temp, 1) +
                ",\"humidity\":" + String(hum, 1) +
                ",\"fire\":\"" + fire + "\"" +
                ",\"water\":" + String(waterVal) +
                ",\"dayNight\":\"" + dayNight + "\"}";
  
  // Send to ESP32
  espSerial.println(json);
  
  // Debug to PC
  Serial.println("Sent to ESP: " + json);
}

void processCommand(String cmd) {
  // Protocol format: "TYPE:INDEX:VALUE"
  // Examples: "SRV:1:90", "FAN:0:1", "LGT:1:255"
  
  cmd.trim();
  
  int firstColon = cmd.indexOf(':');
  int secondColon = cmd.lastIndexOf(':');
  
  if (firstColon == -1) return; // Bad command

  String type = cmd.substring(0, firstColon);
  int index = cmd.substring(firstColon + 1, secondColon).toInt();
  int value = cmd.substring(secondColon + 1).toInt();

  // Debug received command
  Serial.print("CMD: "); Serial.print(type);
  Serial.print(" Idx:"); Serial.print(index);
  Serial.print(" Val:"); Serial.println(value);

  if (type == "SRV") {
    if (index == 1) servo1.write(value);
    if (index == 2) servo2.write(value);
    if (index == 3) servo3.write(value);
  }
  else if (type == "FAN") {
    digitalWrite(fanPin, value ? HIGH : LOW);
  }
  else if (type == "LGT") {
    if (index == 1) analogWrite(light1Pin, value);
    if (index == 2) analogWrite(light2Pin, value);
  }
}