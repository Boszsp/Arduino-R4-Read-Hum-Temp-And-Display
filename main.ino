#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <LCDI2C_Multilingual.h>

LCDI2C_Symbols lcd(0x27, 16, 2);           // I2C address: 0x27; Display size: 16x2

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();

  while (!Serial)
    delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {   // Set to 0x44, 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");
}

void displayToLcd(String line1,String line2){
  lcd.setCursor(0,0); 
   //lcd.println("Temperature:25°C" , line1);
  lcd.println(line1);
  lcd.println(line2);
}

struct tempAndHumStruct {
  float temp;
  float hum;
};

void  getTempAndHum(struct tempAndHumStruct *source){
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (! isnan(t)&&! isnan(h)) {  // check if 'is not a number'
  
 // Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
   source->temp = t;
   source->hum = h;
  } else { 
   // Serial.println("Failed to read temperature");
  }


  delay(1000);

  // Toggle heater enabled state every 30 seconds
  // An ~3.0 degC temperature increase can be noted when heater is enabled
  if (loopCnt >= 30) {
    enableHeater = !enableHeater;
    sht31.heater(enableHeater);
    Serial.print("Heater Enabled State: ");
    if (sht31.isHeaterEnabled())
      Serial.println("ENABLED");
    else
      Serial.println("DISABLED");

    loopCnt = 0;
  }
  loopCnt++;
}

void loop() {
  struct tempAndHumStruct tempAndHum;
  getTempAndHum(&tempAndHum);
  Serial.println("Temp");
  Serial.print(tempAndHum.temp);
  Serial.print("Hum");
  Serial.print(tempAndHum.hum);
  char buff1[16], buff2[16];
  snprintf (buff1, sizeof(buff1), "Temp:   %.2f°C", tempAndHum.temp);
  snprintf (buff2, sizeof(buff2), "Hum :   %.2f", tempAndHum.hum);
  displayToLcd(buff1,buff2);

}
