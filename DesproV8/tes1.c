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

#define hand_in 5
#define IR_TRANSMITTER 13

static const int servoPin = 15;

Servo servo1;

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/ESP32_Pertama/shadow/update"
//#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/ESP32_Pertama/shadow/update/delta"

#define LED_ESP32 16

int msgReceived = 0;
String rcvdPayload;
char sndPayloadOff[512];
char sndPayloadOn[512];

int counter = 0;
bool LED_STATE = false;
bool LED_Flag = false;

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
  pinMode(LED_ESP32, OUTPUT);
  servo1.attach(servoPin);
  pinMode(hand_in, INPUT);

  StaticJsonDocument<200> doc_payloadOn;
  StaticJsonDocument<200> doc_payloadOff;

  doc_payloadOn["state"]["reported"]["status"] = "on";
  doc_payloadOn["state"]["reported"]["status_TV"] = "on";
  serializeJson(doc_payloadOn, sndPayloadOn);

  doc_payloadOff["state"]["reported"]["status"] = "off";
  doc_payloadOff["state"]["reported"]["status_TV"] = "off";
  serializeJson(doc_payloadOff, sndPayloadOff);

  /*
    sprintf(sndPayloadOn, "{\"state\": { \"reported\": { \"status\": \"on\" } }}");
    sprintf(sndPayloadOff, "{\"state\": { \"reported\": { \"status\": \"off\" } }}");
  */
  connectAWS();

  Serial.println("Setting Lamp Status to Off");
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
    StaticJsonDocument<200> sensor_doc;
    DeserializationError error_sensor = deserializeJson(sensor_doc, rcvdPayload);
    const char *sensor = sensor_doc["state"]["status"];

    Serial.print("AWS Says:");
    Serial.println(sensor);

    char sndPayload[512];

    StaticJsonDocument<200> doc_payload;
    //
    //    const char *status_TV = sensor_doc["state"]["status_TV"];
    //
    //    Serial.print("Status_TV adalah :");
    //    Serial.println(status_TV);

    
    doc_payload["state"]["reported"]["status_TV"] = sensor_doc["state"]["status_TV"];
    serializeJson(doc_payload, sndPayload);
    Serial.println("Yang akan dikirim :");
    Serial.println(sndPayload);
    if (sensor != NULL) {
      doc_payload["state"]["reported"]["status"] = sensor_doc["state"]["status"];
      serializeJson(doc_payload, sndPayload);
      Serial.println("Yang akan dikirim :");
      Serial.println(sndPayload);
      if (strcmp(sensor, "on") == 0)
      {
        Serial.println("IF CONDITION");
        Serial.println("Turning Lamp On");
        digitalWrite(LED_ESP32, LOW);
        servo1.write(90);
        //      client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOn);
        client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
      }
      else
      {
        Serial.println("ELSE CONDITION");
        Serial.println("Turning Lamp Off");
        digitalWrite(LED_ESP32, HIGH);
        servo1.write(180);
        //      client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayloadOff);
        client.publish(AWS_IOT_PUBLISH_TOPIC, sndPayload);
      }
    }
    else if
    Serial.println("##############################################");
  }
  client.loop();
  handToggle();
}