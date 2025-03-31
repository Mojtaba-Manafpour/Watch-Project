#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <vector>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); 

#define DHTPIN 33
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LED_FAN 18
#define LED_HEATER 4
#define LED_WATER 32

#define RS 13 
#define EN 12
#define D4 25
#define D5 26
#define D6 27  
#define D7 14  

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

const char* ssid = "UTEL";
const char* password = "1570420246";

WebServer server(80);

float fanThreshold = 25.00;
float heaterThreshold = 20.00;
float waterThreshold = 15.00;

int hours = 12, minutes = 00, seconds = 0;
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
  
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }";
  html += "h2 { color: #4CAF50; }";
  html += "p { font-size: 18px; }";
  html += "span { font-weight: bold; color: #333; }";
  html += "input { padding: 5px; margin: 5px; }";
  html += "button { background: #4CAF50; color: white; padding: 10px; border: none; cursor: pointer; }";
  html += "button:hover { background: #45a049; }";
  html += "</style>";

  html += "<script>function updateData() { fetch('/status').then(response => response.json()).then(data => {";
  html += "document.getElementById('time').innerText = data.time;";
  html += "document.getElementById('temp').innerText = data.temperature + ' C';";
  html += "document.getElementById('fan').innerText = data.fan;";
  html += "document.getElementById('heater').innerText = data.heater;";
  html += "document.getElementById('water').innerText = data.water;";
  html += "});} setInterval(updateData, 1000);</script></head><body>";

  html += "<h2>Greenhouse Control Panel</h2>";
  html += "<p> Time: <span id='time'>--:--:--</span></p>";
  html += "<p> Temperature: <span id='temp'>--.- C</span></p>";
  html += "<p> Fan: <span id='fan'>OFF</span></p>";
  html += "<p> Heater: <span id='heater'>OFF</span></p>";
  html += "<p> Water: <span id='water'>OFF</span></p>";
  
  html += "<form action='/set' method='POST'>";
  html += "Fan Threshold: <input type='number' name='fan' step='0.1' value='" + String(fanThreshold) + "'><br>";
  html += "Heater Threshold: <input type='number' name='heater' step='0.1' value='" + String(heaterThreshold) + "'><br>";
  html += "Water Threshold: <input type='number' name='water' step='0.1' value='" + String(waterThreshold) + "'><br>";
  html += "<button type='submit'> Set Values</button>";
  html += "</form>";

  html += "<h3>Set Watering Schedule</h3>";
  html += "<form action='/add_watering' method='POST'>";
  html += "Start Time: <input type='time' name='start'><br>";
  html += "End Time: <input type='time' name='end'><br>";
  html += "<button type='submit'> Add Watering Time</button>";
  html += "</form>";

  html += "<h3>Current Watering Schedule</h3><ul>";
  for (int i = 0; i < wateringSlots.size(); i++) {
    html += "<li>" + String(wateringSlots[i].startHour) + ":" + String(wateringSlots[i].startMinute) + " - " + 
            String(wateringSlots[i].endHour) + ":" + String(wateringSlots[i].endMinute) + 
            " <a href='/delete_watering?index=" + String(i) + "'><button> Delete</button></a></li>";
  }
  html += "</ul></body></html>";

  server.send(200, "text/html", html);
}

void handleSetValues() {
  if (server.hasArg("fan")) fanThreshold = server.arg("fan").toFloat();
  if (server.hasArg("heater")) heaterThreshold = server.arg("heater").toFloat();
  if (server.hasArg("water")) waterThreshold = server.arg("water").toFloat();

  float temp = dht.readTemperature();
  if (!isnan(temp)) {
    if (temp >= fanThreshold) {
      digitalWrite(LED_FAN, HIGH);
      digitalWrite(LED_HEATER, LOW);
    }
    else if (temp <= fanThreshold && temp >= heaterThreshold) {
      digitalWrite(LED_HEATER, LOW);
      digitalWrite(LED_FAN, LOW);

    }
    else if (temp <= heaterThreshold) {
      digitalWrite(LED_HEATER, HIGH);
      digitalWrite(LED_FAN, LOW);
    }
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

  for (int i = 0; i < wateringSlots.size(); i++) {
    if (hours == wateringSlots[i].startHour && minutes == wateringSlots[i].startMinute) {
      digitalWrite(LED_WATER, HIGH);
    }

    if (hours == wateringSlots[i].endHour && minutes == wateringSlots[i].endMinute) {
      digitalWrite(LED_WATER, LOW);
    }
  }

  sendHtml();
}

void handleDeleteWatering() {
  if (server.hasArg("index")) {
    int index = server.arg("index").toInt();
    if (index >= 0 && index < wateringSlots.size()) {
      wateringSlots.erase(wateringSlots.begin() + index);
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
  
  timeClient.begin();  
  timeClient.setTimeOffset(12600); 

  timeClient.update();
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();

  server.on("/", sendHtml);
  server.on("/status", sendStatus);
  server.on("/set", HTTP_POST, handleSetValues);
  server.on("/add_watering", HTTP_POST, handleAddWatering);
  server.on("/delete_watering", HTTP_GET, handleDeleteWatering);

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
      Serial.print("ESP32 IP Address: ");
      Serial.println(WiFi.localIP());
      
      Serial.print("Temp: ");
      Serial.print(temp);
      Serial.println(" C");

      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temp);
      lcd.print(" C  ");

      if (temp >= fanThreshold) {
      digitalWrite(LED_FAN, HIGH);
      digitalWrite(LED_HEATER, LOW);
      }
      else if (temp <= fanThreshold && temp >= heaterThreshold) {
        digitalWrite(LED_HEATER, LOW);
        digitalWrite(LED_FAN, LOW);
      }
      else if (temp <= heaterThreshold) {
        digitalWrite(LED_HEATER, HIGH);
        digitalWrite(LED_FAN, LOW);
      }
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
