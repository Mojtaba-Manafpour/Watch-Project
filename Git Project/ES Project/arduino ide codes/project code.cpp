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

#define LED_FAN  19   
#define LED_HEATER  23  
#define LED_WATER  0    

#define RS 2 
#define EN 4
#define D4 16 
#define D5 17 
#define D6 5  
#define D7 18  

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int hours = 12, minutes = 30, seconds = 0;
unsigned long prevMillis = 0, prevTempMillis = 0;
float lastTemp = dht.readTemperature();

const char* ssid = "UTEL";  
const char* password = "";  

WebServer server(80);  

void handleRoot() {
  float temp = dht.readTemperature();
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>button {font-size: 20px; margin: 10px;}</style></head><body>";
  html += "<h2>Greenhouse Control</h2>";
  html += "<p>Time: " + String(hours) + ":" + String(minutes) + ":" + String(seconds) + "</p>";
  html += "<p>Temperature: " + String(temp) + " C</p>";
  html += "<button onclick='toggleFan()'>Toggle Fan</button>";
  html += "<button onclick='toggleHeater()'>Toggle Heater</button>";
  html += "<button onclick='toggleWater()'>Toggle Water</button>";
  html += "<script>";
  html += "function toggleFan() {fetch('/fan');}";
  html += "function toggleHeater() {fetch('/heater');}";
  html += "function toggleWater() {fetch('/water');}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

void toggleFan() {
  digitalWrite(LED_FAN, !digitalRead(LED_FAN));
  server.send(200, "text/plain", "Fan toggled");
}

void toggleHeater() {
  digitalWrite(LED_HEATER, !digitalRead(LED_HEATER));
  server.send(200, "text/plain", "Heater toggled");
}

void toggleWater() {
  digitalWrite(LED_WATER, !digitalRead(LED_WATER));
  server.send(200, "text/plain", "Water toggled");
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

  server.on("/", handleRoot);
  server.on("/fan", toggleFan);
  server.on("/heater", toggleHeater);
  server.on("/water", toggleWater);

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

      digitalWrite(LED_FAN, temp >= 20 ? HIGH : LOW);
      digitalWrite(LED_HEATER, temp <= 18 ? HIGH : LOW);
    }
  }
}
