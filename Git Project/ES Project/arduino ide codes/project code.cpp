#include <Wire.h>
#include <LiquidCrystal.h>

#define RS 2 
#define EN 4
#define D4 16 
#define D5 17 
#define D6 5  
#define D7 18  

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int hours = 12, minutes = 30, seconds = 0;
unsigned long prevMillis = 0;

void setup() {
  lcd.begin(16, 2);  
  lcd.backlight();
}

void loop() {
  if (millis() - prevMillis >= 500) { 
    prevMillis = millis();
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
}