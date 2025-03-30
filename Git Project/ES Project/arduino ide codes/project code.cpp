#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <vector>

#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LED_FAN 19
#define LED_HEATER 23
#define LED_WATER 0

#define RS 2 
#define EN 4
#define D4 16 
#define D5 17 
#define D6 5  
#define D7 18  

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const char* ssid = "UTEL";
const char* password = "";

WebServer server(80);

float fanThreshold = 30.0;
float heaterThreshold = 25.0;
float waterThreshold = 15.0;

int hours = 12, minutes = 30, seconds = 0;
unsigned long prevMillis = 0, prevTempMillis = 0;
float lastTemp = dht.readTemperature();

struct WateringSlot {
  int startHour;
  int startMinute;
  int endHour;
  int endMinute;
};

std::vector<WateringSlot> wateringSlots;

void sendHtml() {
  String html = "<!DOCTYPE html><html><head><title>Greenhouse Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<script>function updateData() { fetch('/status').then(response => response.json()).then(data => {";
  html += "document.getElementById('time').innerText = data.time;";
  html += "document.getElementById('temp').innerText = data.temperature + ' C';";
  html += "document.getElementById('fan').innerText = data.fan;";
  html += "document.getElementById('heater').innerText = data.heater;";
  html += "document.getElementById('water').innerText = data.water;";
  html += "});} setInterval(updateData, 2000);</script></head><body>";
  html += "<h2>Greenhouse Control Panel</h2>";
  html += "<p>Time: <span id='time'>--:--:--</span></p>";
  html += "<p>Temperature: <span id='temp'>--.- C</span></p>";
  html += "<p>Fan: <span id='fan'>OFF</span></p>";
  html += "<p>Heater: <span id='heater'>OFF</span></p>";
  html += "<p>Water: <span id='water'>OFF</span></p>";
  html += "<form action='/set' method='POST'>";
  html += "Fan Threshold: <input type='number' name='fan' step='0.1' value='" + String(fanThreshold) + "'><br>";
  html += "Heater Threshold: <input type='number' name='heater' step='0.1' value='" + String(heaterThreshold) + "'><br>";
  html += "Water Threshold: <input type='number' name='water' step='0.1' value='" + String(waterThreshold) + "'><br>";
  html += "<input type='submit' value='Set Values'>";
  html += "</form>";

  html += "<h3>Set Watering Schedule</h3>";
  html += "<form action='/add_watering' method='POST'>";
  html += "Start Time: <input type='time' name='start'><br>";
  html += "End Time: <input type='time' name='end'><br>";
  html += "<input type='submit' value='Add Watering Time'>";
  html += "</form>";

  html += "<h3>Current Watering Schedule</h3><ul>";
  for (const auto& time : wateringSlots) {
    html += "<li>" + String(time.startHour) + ":" + String(time.startMinute) + " - " + String(time.endHour) + ":" + String(time.endMinute) + "</li>";
  }
  html += "</ul></body></html>";

  server.send(200, "text/html", html);
}

void handleSetValues() {
  if (server.hasArg("fan")) fanThreshold = server.arg("fan").toFloat();
  if (server.hasArg("heater")) heaterThreshold = server.arg("heater").toFloat();
  if (server.hasArg("water")) waterThreshold = server.arg("water").toFloat();
  
  // کنترل LEDها بر اساس مقدار جدید
  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    digitalWrite(LED_FAN, temp >= fanThreshold ? HIGH : LOW);
    digitalWrite(LED_HEATER, temp <= heaterThreshold ? HIGH : LOW);
    digitalWrite(LED_WATER, temp <= waterThreshold ? HIGH : LOW);
  }

  sendHtml();
}

void handleAddWatering() {
  if (server.hasArg("start") && server.hasArg("end")) {
    String startTime = server.arg("start");
    String endTime = server.arg("end");
    WateringSlot newTime;
    sscanf(startTime.c_str(), "%d:%d", &newTime.startHour, &newTime.startMinute);
    sscanf(endTime.c_str(), "%d:%d", &newTime.endHour, &newTime.endMinute);
    wateringSlots.push_back(newTime);
  }
  sendHtml();
}

void sendStatus() {
  String json = "{";
  json += "\"time\":\"" + String(hours) + ":" + String(minutes) + ":" + String(seconds) + "\",";
  json += "\"temperature\":\"" + String(lastTemp) + "\",";
  json += "\"fan\":\"" + String(fanThreshold) + "\",";
  json += "\"heater\":\"" + String(heaterThreshold) + "\",";
  json += "\"water\":\"" + String(waterThreshold) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  dht.begin();
  pinMode(LED_FAN, OUTPUT);
  pinMode(LED_HEATER, OUTPUT);
  pinMode(LED_WATER, OUTPUT);
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Time: --:--:--");
  lcd.setCursor(0, 1);
  lcd.print("Temp: --.- C");

  server.on("/", sendHtml);
  server.on("/status", sendStatus);
  server.on("/set", HTTP_POST, handleSetValues);
  server.on("/add_watering", HTTP_POST, handleAddWatering);

  server.begin();
  Serial.println("\nConnected! Open http://" + WiFi.localIP + "in your browser.");
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();

  if (currentMillis - prevMillis >= 1000) {
    prevMillis = currentMillis;
    seconds++;
    if (seconds == 60) { seconds = 0; minutes++; }
    if (minutes == 60) { minutes = 0; hours++; }
    if (hours == 24) { hours = 0; }

    lcd.setCursor(0, 0);
    char timeStr[9];
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
    lcd.print("Time: ");
    lcd.print(timeStr);
  }

  if (currentMillis - prevTempMillis >= 1000) {
    prevTempMillis = currentMillis;
    float temp = dht.readTemperature();

    if (!isnan(temp) && temp != lastTemp) {
      lastTemp = temp;
      Serial.print("Temp: ");
      Serial.print(temp);
      Serial.println(" C");

      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temp);
      lcd.print(" C  ");

      digitalWrite(LED_FAN, temp >= fanThreshold ? HIGH : LOW);
      digitalWrite(LED_HEATER, temp <= heaterThreshold ? HIGH : LOW);
      digitalWrite(LED_WATER, temp <= waterThreshold ? HIGH : LOW);
    }
  }

  for (int i = 0; i < wateringSlots.size(); i++) {
    if (hours == wateringSlots[i].startHour && minutes == wateringSlots[i].startMinute) {
      digitalWrite(LED_WATER, HIGH);
    }

    if (hours == wateringSlots[i].endHour && minutes == wateringSlots[i].endMinute) {
      digitalWrite(LED_WATER, LOW);
    }
  }
}
