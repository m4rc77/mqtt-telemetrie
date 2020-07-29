// Board Libraries
#include <ESP8266WiFi.h>

// Required libraries:
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// The Wemos D1 mini's pins do not map to arduino pin numbers accurately ... see
// https://github.com/esp8266/Arduino/issues/1243

#include "config.h"
// File config.h should contain the configuration in the following form ...
/*
const char *ssid       = "your WLAN SSID";
const char *password   = "your WLAN password";
const char *mqttServer = "the broker url";
const char *mqttTopicStatus  = "your Topic/status";
const char *mqttTopicCounter = "your Topic/counter";
const char *mqttTopicValue0 = "your Topic/value/0";
const char *mqttTopicValue1 = "your Topic/value/1";
const char *mqttTopicValue2 = "your Topic/value/2";
const char *mqttTopicValue3 = "your Topic/value/3";
*/

const char *lastWillMessage = "0"; // the last will message show clients that we are offline
const int PUBLISH_DELAY = 1000; //milliseconds
const int TIMEOUT = 3; // seconds

// Variables ...
long lastPublish = 0;
long counter = 0;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);
Adafruit_ADS1115 ads1115(0x48);  // construct an ads1115 at address 0x48 (default)

void setup() {
  Serial.begin(9600);

  // setup io-pins
  pinMode(LED_BUILTIN, OUTPUT);  // initialize onboard LED as output
  ledOff();

  setupWifi();
  setupMqtt();
  setupAdc();
}

void setupWifi() {
  delay(10);
  Serial.println();Serial.print("Connecting to ");Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");Serial.println(WiFi.localIP());
}

void setupMqtt() {
  mqttClient.setServer(mqttServer, 1883);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(TIMEOUT);
  mqttClient.setSocketTimeout(TIMEOUT);
}

void setupAdc() {
  ads1115.begin();  // Initialize ads1115  
  ads1115.setGain(GAIN_ONE);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = String(topic);
  Serial.print("Message arrived [");Serial.print(topicStr);Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
/*
  if (topicStr == mqttTopicLedSet) {
    Serial.println("Got LED switching command ...");
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
      ledOn();
    } else {
      ledOff();
    }
    mqttPublishLedState();
  }
  */
}

void loop() {
  checkWifi();
  checkMqtt();
  mqttClient.loop();

  if(lastPublish + PUBLISH_DELAY <= millis()) {
    counter++;
    mqttPublishData();
    lastPublish = millis();
  }
}

void checkWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();  
  }
}

void checkMqtt() {
  if (!mqttClient.connected()) {
    while (!mqttClient.connected()) {
      Serial.print("Attempting to open MQTT connection...");
      // connect with last will (QoS=1, retain=true, ) ...
      if (mqttClient.connect("ESP8266_Telemetrie_Client", mqttTopicStatus, 1, true, lastWillMessage)) {
        Serial.println("connected");
        //mqttClient.subscribe(mqttTopicIn);
        flashLed();
        publishString(mqttTopicStatus, "1");
      } else {
        Serial.print("MQTT connection failed, retry count: ");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 1 seconds");
        delay(1000);
      }
    }
  }  
}

void mqttPublishData() {
  int16_t adc0 = ads1115.readADC_SingleEnded(0);
  int16_t adc1 = ads1115.readADC_SingleEnded(1);
  int16_t adc2 = ads1115.readADC_SingleEnded(2);
  int16_t adc3 = ads1115.readADC_SingleEnded(3);
  Serial.print("AIN0: "); Serial.println(adc0);
  Serial.print("AIN1: "); Serial.println(adc1);
  Serial.print("AIN2: "); Serial.println(adc2);
  Serial.print("AIN3: "); Serial.println(adc3);

  float v0 = adcValue2milliVolt(adc0);
  float v1 = adcValue2milliVolt(adc1);
  float v2 = adcValue2milliVolt(adc2);
  float v3 = adcValue2milliVolt(adc3);

  // correct mesured values due to resistors used to scale down high voltages ...
  /*
  v0 *= 1.3929;
  v1 *= 2.7857;
  v2 *= 4.0303;
  v3 *= 5.5455;
  */

  // calculate voltages per cell by subtracting the next lower value ...
  //v3 -= v2;
  //v2 -= v1;
  //v1 -= v0;
  
  publishLong(mqttTopicCounter, counter);
  publishFloat(mqttTopicValue0, v0 / 1000);
  publishFloat(mqttTopicValue1, v1 / 1000);
  publishFloat(mqttTopicValue2, v2 / 1000);
  publishFloat(mqttTopicValue3, v3 / 1000);
  
  shortFlashLed();
}

// =================================================================================================================================
// Helper methods 
// =================================================================================================================================
void publishLong(const char* topic, long value) {
  Serial.print("Publishing ");Serial.print(value);Serial.print(" to topic ");Serial.print(topic);Serial.print(" (");Serial.print(mqttServer);Serial.println(")");
  String valueStr = String(value); //converting value to a string 
  char charBufValue[valueStr.length() + 1];
  valueStr.toCharArray(charBufValue, valueStr.length() + 1); //packaging up the data to publish to mqtt ...
  mqttClient.publish(topic, charBufValue);
}

void publishFloat(const char* topic, float value) {
  Serial.print("Publishing ");Serial.print(value);Serial.print(" to topic ");Serial.print(topic);Serial.print(" (");Serial.print(mqttServer);Serial.println(")");
  String valueStr = String(value); //converting value to a string 
  char charBufValue[valueStr.length() + 1];
  valueStr.toCharArray(charBufValue, valueStr.length() + 1); //packaging up the data to publish to mqtt ...
  mqttClient.publish(topic, charBufValue);
}

void publishString(const char* topic, String valueStr) {
  Serial.print("Publishing ");Serial.print(valueStr);Serial.print(" to topic ");Serial.print(topic);Serial.print(" (");Serial.print(mqttServer);Serial.println(")");
  char charBufValue[valueStr.length() + 1];
  valueStr.toCharArray(charBufValue, valueStr.length() + 1); //packaging up the data to publish to mqtt ...
  mqttClient.publish(topic, charBufValue);
}

float adcValue2milliVolt(int16_t adcValue) {
  // GAIN_TWOTHIRDS: +/- 6.144V  1 bit = 0.1875mV
  // GAIN_ONE: +/- 4.096V  1 bit = 0.125mV
  // GAIN_TWO: +/- 2.048V  1 bit = 0.0625mV
  // GAIN_FOUR: +/- 1.024V  1 bit = 0.03125mV
  // GAIN_EIGHT: +/- 0.512V  1 bit = 0.015625mV
  // GAIN_SIXTEEN: +/- 0.256V  1 bit = 0.0078125mV

  // we use GAIN_ONE --> 0.125mV / bit ...
  return adcValue * 0.125;
}

void ledOn() {
  digitalWrite(LED_BUILTIN, LOW);
}

void ledOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}

void shortFlashLed() {
    ledOn();
    delay(10);
    ledOff();
} 

void flashLed() {
  for (int i=0; i < 5; i++){
      ledOn();
      delay(50);
      ledOff();
      delay(50);
   }  
}  
// EOF =============================================================================================================================
