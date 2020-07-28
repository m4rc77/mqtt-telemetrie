// Board Libraries
#include <ESP8266WiFi.h>

// Required libraries:
#include <PubSubClient.h>

// The Wemos D1 mini's pins do not map to arduino pin numbers accurately ... see
// https://github.com/esp8266/Arduino/issues/1243

#include "config.h"
// File config.h sould contain the configuration in the following form ...
/*
const char *ssid       = "your WLAN SSID";
const char *password   = "your WLAN password";
const char *mqttServer = "the broker url";

TODO
*/
const char *lastWillMessage = "0"; // the last will message show clients that we are offline
const int PUBLISH_DELAY = 1000; //milliseconds
const int TIMEOUT = 3; // seconds

// Variables ...
long lastPublish = 0;

long counter = 0;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(9600);

  // setup io-pins
  pinMode(LED_BUILTIN, OUTPUT);  // initialize onboard LED as output
  ledOff();

  setupWifi();
  setupMqtt();
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
  publishLong(mqttTopicCounter, counter);
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

void publishString(const char* topic, String valueStr) {
  Serial.print("Publishing ");Serial.print(valueStr);Serial.print(" to topic ");Serial.print(topic);Serial.print(" (");Serial.print(mqttServer);Serial.println(")");
  char charBufValue[valueStr.length() + 1];
  valueStr.toCharArray(charBufValue, valueStr.length() + 1); //packaging up the data to publish to mqtt ...
  mqttClient.publish(topic, charBufValue);
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