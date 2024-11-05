// Lib For LED Matrix Display
#include "Arduino_LED_Matrix.h"
#include "grid_matrix.h"
ArduinoLEDMatrix matrix;

// Lib For MQTT
#include <ArduinoMqttClient.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

///////please enter your sensitive data in the Secret tab/arduino_secrets.h


const char broker[] = "broker.hivemq.com";
int port = 1883;
const char send_topic[] = "/242/send";

//set interval for sending messages (milliseconds)
const long interval = 1000;
unsigned long previousMillis = 0;

int status = WL_IDLE_STATUS;  // the WiFi radio's status


// Lib For Sensor & LCD Display
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <LCDI2C_Multilingual.h>

LCDI2C_Symbols lcd(0x27, 16, 2);  // I2C address: 0x27; Display size: 16x2

bool enableHeater = false;
uint8_t loopCnt = 0;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void loadding(){
  matrix.renderBitmap(wifi_icon_connecting, 8, 12);
}

void setupNetwork() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    loadding();
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
}

void setupTempSensor() {
  Serial.println("SHT31 test");
  if (!sht31.begin(0x44)) {  // Set to 0x44, 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  Serial.print("Heater Enabled State: ");
  if (sht31.isHeaterEnabled())
    Serial.println("ENABLED");
  else
    Serial.println("DISABLED");
}

void setupMQTT() {
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  setupNetwork();

  Serial.println("You're connected to the network");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1)
      ;
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  matrix.begin();
  matrix.clear();

  while (!Serial)
    delay(10);  // will pause Zero, Leonardo, etc until serial console opens
  setupTempSensor();
  // Setup LCD Display
  lcd.init();
  lcd.backlight();
  setupMQTT();
}

void displayToLcd(String line1, String line2) {
  lcd.setCursor(0, 0);
  //lcd.println("Temperature:25°C" , line1);
  lcd.println(line1);
  lcd.println(line2);
}

struct tempAndHumStruct {
  float temp;
  float hum;
};

void getTempAndHum(struct tempAndHumStruct *source) {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();

  if (!isnan(t) && !isnan(h)) {  // check if 'is not a number'

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

void publishMessageMQTT(String mss) {
  // call poll() regularly to allow the library to send MQTT keep alive which
  // avoids being disconnected by the broker
  mqttClient.poll();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;
    mqttClient.beginMessage(send_topic);
    mqttClient.print(mss);
    mqttClient.endMessage();
  }
}

void checkWifiIsConnecting() {
  if (WiFi.status() == WL_CONNECTED) {
    matrix.renderBitmap(wifi_icon, 8, 12);
  } else {
    matrix.renderBitmap(wifi_icon_not_connected, 8, 12);
  }
}

void loop() {


  struct tempAndHumStruct tempAndHum;
  checkWifiIsConnecting();
  getTempAndHum(&tempAndHum);
  Serial.println("Temp");
  Serial.print(tempAndHum.temp);
  Serial.print("Hum");
  Serial.print(tempAndHum.hum);
  char buff1[20], buff2[20], buff3[50];
  snprintf(buff1, sizeof(buff1), "Temp:   %.2f°C", tempAndHum.temp);
  snprintf(buff2, sizeof(buff2), "Hum :   %.2f", tempAndHum.hum);
  displayToLcd(buff1, buff2);
  snprintf(buff3, sizeof(buff3), "{\"temp\":\"%.2f\",\"hum\":\"%.2f\"}", tempAndHum.temp, tempAndHum.hum);
  publishMessageMQTT(buff3);
}
