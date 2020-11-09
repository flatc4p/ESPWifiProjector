/*
  ESP8266 Blink by Simon Peter
  Blink the blue LED on the ESP-01 module
  This example code is in the public domain

  The blue LED on the ESP-01 module is connected to GPIO1
  (which is also the TXD pin; so we cannot use Serial.print() at the same time)

  Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRutils.h>
#include "secrets.h"


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
const char* MQTT_BROKER = MQTT_BROKER_IP;
uint16_t MQTT_PORT = 1883;
const char* MQTT_username = MQTT_USER;
const char* MQTT_password = MQTT_PASSWD;

const uint16_t kIRLED = 0;

bool bProjectorOn;
char message[10];

IRsend irsend(kIRLED);

WiFiClient wifi;    //WiFi connection handle (connect to, disconnect from WiFi)
PubSubClient mqtt(wifi);  //mqtt handle (connect to mqtt broker subscribe, publish to mqtt topics)

void setup() {
  //pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(kIRLED, OUTPUT);
  digitalWrite(kIRLED, HIGH);
  delay(5000);
  digitalWrite(kIRLED, LOW);
  Serial.begin(115200);
  irsend.begin();
  setup_wifi();
  setup_mqtt();
  bProjectorOn = false;
}

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nConnected, IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  // notify of message via serial port (debugging only)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //handle if it was sent to the correct topic
  if (strcmp(topic, "/esp/projector")  == 0 && length < 50) {
    Serial.println("projector command received");
    //retrieve payload:
    for (int i = 0; i < length; i++) {
      message[i] = (char)payload[i];
    }
    Serial.print("Message: ");
    Serial.println(message);
    //identify request:
    if (strcmp(message, "on") == 0 && !bProjectorOn) {
      Serial.println("Turning on projector!");
      irsend.sendNEC(0x10C8E11E); //turn projector on
      bProjectorOn = true;
    }
    if (strcmp(message, "off") == 0 && bProjectorOn) {
      Serial.println("Shutting off projector!");
      irsend.sendNEC(0x10C8E11E); //send first off signal
      delay(750);                  //wait for call for second off signal
      irsend.sendNEC(0x10C8E11E); //send second off signal
      bProjectorOn = false;
    }
    if (strcmp(message, "state") == 0) {
      mqtt.publish("/esp/projector/state", bProjectorOn?"on":"off");
    }
    //clearing message:
    for (int i = 0; i < 10; i++) {
      message[i] = 0;
    }
  }

  //  // Switch on the LED if an 1 was received as first character
  //  if ((char)payload[0] == '1') {
  //    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  //    // but actually the LED is on; this is because
  //    // it is active low on the ESP-01)
  //  } else {
  //    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  //  }
}

void setup_mqtt() {
  delay(10);
  Serial.println("Setting up mqtt");
  mqtt.setClient(wifi);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(callback);
}
void reconnect() {
  while (!mqtt.connected()) {
    Serial.println("Reconnecting MQTT...");
    if (!mqtt.connect("ESP-01", MQTT_username, MQTT_password)) {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
  mqtt.subscribe("/esp/projector");
  Serial.println("MQTT Connected...");
}

// the loop function runs over and over again forever
void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  //digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  // but actually the LED is on; this is because
  // it is active low on the ESP-01)
  //delay(1000);                      // Wait for a second
  //digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(100);                      // Wait for 10 miliseconds
  mqtt.loop();
}
