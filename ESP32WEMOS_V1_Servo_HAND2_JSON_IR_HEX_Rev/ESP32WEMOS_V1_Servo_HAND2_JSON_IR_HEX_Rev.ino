/*
  Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <Servo.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>


#define hand_in 5
//#define IR_TRANSMITTER 13 //ini adalah file original
#define IR_TRANSMITTER 4 //ini adalah versi esp32wemos mini
#define SENDING_REPEATS 20

//static const int servoPin = 15; //ini adalah file original
static const int servoPin = 18; //ini adalah versi esp32wemos mini

Servo servo1;
IRsend irsend(IR_TRANSMITTER);

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/ESP32_Pertama/shadow/update"
//#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/ESP32_Pertama/shadow/update/delta"

#define LED_ESP32 16

int msgReceived = 0;
String rcvdPayload;
char sndPayloadOff[512];
char sndPayloadOn[512];

bool sendMQTT_Report = false;

int counter = 0;
bool LED_STATE = false;
bool LED_Flag = false;
char start_HEX_TV[10] = "20DF10EF";
char start_HEX_AC[10] = "88C0051";
int TV_Brand = 1;
int AC_Brand = 1;

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("");
  Serial.println("###################### Starting Execution ########################");
  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  msgReceived = 1;
  rcvdPayload = payload;

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *sensor = doc["state"]["status"];
  Serial.print("AWS Says:");
  Serial.println(sensor);
}

void handToggle() {
  int hand = !digitalRead(hand_in);
  if (hand == 1) {
    counter++;
  }
  else if (hand == 0) {
    counter = 0;
    LED_Flag = false;
  }
  //  Serial.print(" Counter:");
  //  Serial.println(counter);
  if (counter >= 400 && LED_Flag == false) {
    LED_STATE = !LED_STATE;
    digitalWrite(2, LED_STATE);
    if (LED_STATE) {
      digitalWrite(LED_ESP32, LOW);
      servo1.write(90);
    }
    else {
      digitalWrite(LED_ESP32, HIGH);
      servo1.write(180);
    }
    LED_Flag = true;
  }
}

void setup() {
  Serial.begin(115200);
  irsend.begin();
  pinMode(LED_ESP32, OUTPUT);
  servo1.attach(servoPin, 2);
  pinMode(hand_in, INPUT);

  StaticJsonDocument<200> doc_payloadOn;
  StaticJsonDocument<200> doc_payloadOff;

  doc_payloadOn["state"]["reported"]["status"] = "on";
  doc_payloadOn["state"]["reported"]["status_TV"] = "on";
  doc_payloadOn["state"]["reported"]["status_AC"] = "on";
  serializeJson(doc_payloadOn, sndPayloadOn);

  doc_payloadOff["state"]["reported"]["status"] = "off";
  doc_payloadOff["state"]["reported"]["status_TV"] = "off";
  doc_payloadOff["state"]["reported"]["HEX_TV"] = start_HEX_TV;
  doc_payloadOff["state"]["reported"]["TV_Brand"] = TV_Brand;
  doc_payloadOff["state"]["reported"]["status_AC"] = "off";
  doc_payloadOff["state"]["reported"]["HEX_AC"] = start_HEX_AC;
  doc_payloadOff["state"]["reported"]["AC_Brand"] = AC_Brand;
  serializeJson(doc_payloadOff, sndPayloadOff);
  connectAWS();

  Serial.println("Setting ALL Status to Off");
  servo1.write(180);
  digitalWrite(LED_ESP32, HIGH);
  client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOff);

  Serial.println("##############################################");
}

void loop() {
  if (msgReceived == 1)
  {
    //      This code will run whenever a message is received on the SUBSCRIBE_TOPIC_NAME Topic
    delay(100);
    msgReceived = 0;
    Serial.print("Received Message:");
    Serial.println(rcvdPayload);
    StaticJsonDocument<1000> sensor_doc;
    DeserializationError error_sensor = deserializeJson(sensor_doc, rcvdPayload);
    const char *sensor = sensor_doc["state"]["status"];
    const char *status_TV = sensor_doc["state"]["status_TV"];
    const char *HEX_TV = sensor_doc["state"]["HEX_TV"];
    const char *status_AC = sensor_doc["state"]["status_AC"];
    const char *HEX_AC = sensor_doc["state"]["HEX_AC"];
    int TV_Brand = sensor_doc["state"]["TV_Brand"];
    int AC_Brand = sensor_doc["state"]["AC_Brand"];


    Serial.print("AWS Says:");
    Serial.println(sensor);

    char sndPayload[512];

    StaticJsonDocument<200> doc_payload;

    if (sensor != NULL) {
      doc_payload["state"]["reported"]["status"] = sensor_doc["state"]["status"];
      sendMQTT_Report = true;
      serializeJson(doc_payload, sndPayload);
      Serial.println("Yang akan dikirim :");
      Serial.println(sndPayload);
      if (strcmp(sensor, "on") == 0)
      {
        Serial.println("IF CONDITION");
        Serial.println("Turning Lamp On");
        digitalWrite(LED_ESP32, LOW);
        servo1.write(90);
        //        client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
      }
      else
      {
        Serial.println("ELSE CONDITION");
        Serial.println("Turning Lamp Off");
        digitalWrite(LED_ESP32, HIGH);
        servo1.write(180);
        //        client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
      }
    }
    if (status_TV != NULL) {
      doc_payload["state"]["reported"]["status_TV"] = sensor_doc["state"]["status_TV"];
      unsigned long code = strtoul(start_HEX_TV, NULL, 16);
      if (HEX_TV != NULL) {
        doc_payload["state"]["reported"]["HEX_TV"] = sensor_doc["state"]["HEX_TV"];
        unsigned long code = strtoul(HEX_TV, NULL, 16);
      }
      
      if (TV_Brand != NULL){
        doc_payload["state"]["reported"]["TV_Brand"] = sensor_doc["state"]["TV_Brand"];
      }
      else{
        TV_Brand = 1;
        doc_payload["state"]["reported"]["TV_Brand"] = TV_Brand;
      }
      
      if (TV_Brand == 1){
        irsend.sendNEC(code, 32);
      }
      else if (TV_Brand == 2){
        irsend.sendSony(code, 20); 
      }
      sendMQTT_Report = true;
      serializeJson(doc_payload, sndPayload);
      Serial.println("Yang akan dikirim :");
      Serial.println(sndPayload);
      //      client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
    }
    if (status_AC != NULL) {
      doc_payload["state"]["reported"]["status_AC"] = sensor_doc["state"]["status_AC"];
      unsigned long codeAC = strtoul(start_HEX_AC, NULL, 16);
      if (HEX_AC != NULL) {
        doc_payload["state"]["reported"]["HEX_AC"] = sensor_doc["state"]["HEX_AC"];
        unsigned long codeAC = strtoul(HEX_AC, NULL, 16);
      }
      
      if (AC_Brand != NULL){
        doc_payload["state"]["reported"]["AC_Brand"] = sensor_doc["state"]["AC_Brand"];
      }
      else{
        AC_Brand = 1;
        doc_payload["state"]["reported"]["AC_Brand"] = AC_Brand;
      }
      
      if (AC_Brand == 1){
        irsend.sendLG2(codeAC, 28);
      }
      else if (AC_Brand == 2){
        irsend.sendSony(codeAC, 20); 
      }
      sendMQTT_Report = true;
      serializeJson(doc_payload, sndPayload);
      Serial.println("Yang akan dikirim :");
      Serial.println(sndPayload);
      //      client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
    }
    if (sendMQTT_Report)client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
    sendMQTT_Report = false;
    Serial.print("-----MQTT SEND----");
    Serial.println("##############################################");
  }
  client.loop();
  handToggle();
}
