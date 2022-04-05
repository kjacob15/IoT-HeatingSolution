#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define AWS_IOT_PUBLISH_TOPIC "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
#define Temperature_PIN 14
#define Heating 25
#define FirstMotor 26
#define secondMotor 5

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
OneWire oneWire(T_PIN);
DallasTemperature sensor(&oneWire);

String msg = "0";
// State variables
bool t1 = false;
bool t2 = false;
bool b1 = false;
bool b2 = false;
int count = 0;

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("Not Connected to Wifi");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT \n");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("");
  Serial.println("AWS IoT Connected!");
}

void publishMessage(float tempinC)
{
  Serial.print("Temperature = ");
  Serial.print(tempinC);
  Serial.println("ºC");
  StaticJsonDocument<200> doc;
  // doc["time"] = millis();
  doc["Temperature"] = tempinC;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload)
{
  Serial.println("incoming: " + topic + " - " + payload);

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  //  const char *message = doc["message"];
  msg = doc["field1"].as<String>();
  ;
  //  Serial.println(message);
  action(msg);
}
void action(String msg)
{
  if (msg == "1")
  {
    t1 = true;
    count = 0;
    digitalWrite(Heating, HIGH);
    // checkTemp();
    Serial.print("Heating ON");
  }
  else if (msg == "2")
  {
    t2 = true;
    t1 = false;
    count = 0;
    Serial.println("2nd Trigger received!");
  }
  else
  {
    Serial.println(" LED's OFF!");
    delay(500);
    digitalWrite(FirstMotor, LOW);
    digitalWrite(secondMotor, LOW);
    digitalWrite(Heating, LOW);
  }
}
void checkTemp()
{
  sensor.requestTemperatures();
  float tempinC = sensor.getTempCByIndex(0);
  if (t1 == true && tempinC >= 35.00)
  {
    publishMessage(tempinC);
    trigger1();
  }
  else if (t2 == true && tempinC >= 40.00)
  {
    publishMessage(tempinC);
    trigger2();
  }
  Serial.print("Temperature = ");
  Serial.print(tempinC);
  Serial.println("ºC");
}

void trigger1()
{
  if (count == 0 && b1 == false)
  {
    digitalWrite(FirstMotor, HIGH);
    delay(2000);
    digitalWrite(FirstMotor, LOW);
    b1 = true;
  }
  else
  {
    count++;
    if (count == 10)
    {
      b1 = false;
      count = 0;
    }
  }
}

void trigger2()
{
  if (count == 0 && b2 == false)
  {
    digitalWrite(secondMotor, HIGH);
    delay(2000);
    digitalWrite(secondMotor, LOW);
    b2 = true;
  }
  else
  {
    count++;
    if (count == 10)
    {
      b2 = false;
      count = 0;
    }
  }
}

void setup()
{
  // Set LED as output
  pinMode(FirstMotor, OUTPUT);
  pinMode(secondMotor, OUTPUT);
  pinMode(Heating, OUTPUT);

  Serial.begin(9600);
  connectAWS();
}

void loop()
{
  client.loop();
  checkTemp();
  delay(10000);
}
