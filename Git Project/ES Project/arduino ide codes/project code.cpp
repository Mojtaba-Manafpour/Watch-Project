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

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  pinMode(LED_FAN, OUTPUT);
  pinMode(LED_HEATER, OUTPUT);
  pinMode(LED_WATER, OUTPUT);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Temp: --.- C");
}

void loop() {
  float temp = dht.readTemperature(); 
  temp = dht.getTemperature();

  if (isnan(temp)) {
    Serial.println("temp error");
    return;
  }

  Serial.print("temp: ");
  Serial.print(temp);
  Serial.println(" C");

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temp);
  lcd.print(" C");

  if (temp >= 20) {  
    digitalWrite(LED_FAN, HIGH);
  } else {
    digitalWrite(LED_FAN, LOW);
  }

  if (temp <= 18) {  
    digitalWrite(LED_HEATER, HIGH);
  } else {
    digitalWrite(LED_HEATER, LOW);
  }

  delay(2000);
}
