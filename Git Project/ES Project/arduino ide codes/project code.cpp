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
unsigned long prevMillis = 0;
unsigned long prevTempMillis = 0;
float lastTemp = dht.readTemperature(); 

void setup() {
  Serial.begin(115200);
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
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - prevMillis >= 1000) { 
    prevMillis = currentMillis;
    seconds++;
    
    if (seconds == 60) {
      seconds = 0;
      minutes++;
    }
    if (minutes == 60) {
      minutes = 0;
      hours++;
    }
    if (hours == 24) {
      hours = 0;
    }

    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    char timeStr[9];  
    sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
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
