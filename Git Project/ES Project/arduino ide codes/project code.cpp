#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

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

float fanThreshold = 25.0;    
float heaterThreshold = 20.0;
float waterThreshold = 15.0;  

int hours = 12, minutes = 30, seconds = 0;
unsigned long prevMillis = 0, prevTempMillis = 0;
float lastTemp = -100;

void sendHtml() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <title>Greenhouse Control</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <script>
          function updateData() {
            fetch("/status")
              .then(response => response.json())
              .then(data => {
                document.getElementById("time").innerText = data.time;
                document.getElementById("temp").innerText = data.temperature + " C";
                document.getElementById("fan").innerText = data.fan;
                document.getElementById("heater").innerText = data.heater;
                document.getElementById("water").innerText = data.water;
              });
          }
          setInterval(updateData, 2000);
        </script>
      </head>
      <body>
        <h2>Greenhouse Control Panel</h2>
        <p>Time: <span id="time">--:--:--</span></p>
        <p>Temperature: <span id="temp">--.- C</span></p>
        <p>Fan: <span id="fan">OFF</span></p>
        <p>Heater: <span id="heater">OFF</span></p>
        <p>Water: <span id="water">OFF</span></p>
      </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void sendStatus() {
  float temp = dht.readTemperature();
  String json = "{";
  json += "\"time\":\"" + String(hours) + ":" + String(minutes) + ":" + String(seconds) + "\",";
  json += "\"temperature\":\"" + String(lastTemp) + "\",";
  if (!isnan(temp)) {
    digitalWrite(LED_FAN, (temp >= fanThreshold) ? HIGH : LOW);
    digitalWrite(LED_HEATER, (temp <= heaterThreshold) ? HIGH : LOW);
    digitalWrite(LED_WATER, (temp <= waterThreshold) ? HIGH : LOW);
}
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
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  dht.begin();
  pinMode(LED_FAN, OUTPUT);
  pinMode(LED_HEATER, OUTPUT);
  pinMode(LED_WATER, OUTPUT);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Time: --:--:--");
  lcd.setCursor(0, 1);
  lcd.print("Temp: --.- C");

  server.on("/", sendHtml);
  server.on("/status", sendStatus);

  server.begin();
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

  if (currentMillis - prevTempMillis >= 2000) {  
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
}
