/*
 * Includes
 */
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
//#include "FS.h" // FOR SPIFFS
/*
 * Definitions  
 */
#define CONV_FACTOR 0.00812
const int PulsePin = 13;
const int LedPin = 16;
//#define PulsePin 5 // Input for the external interrupt to count with
//unsigned long int avgValue;     //Store the average value of the sensor feedback
//int i=0;

//const int  PulsePin = 5;
/* 
 * WiFi Settings 
 */
const char *ssid =  "SSID";
const char *pass =  "PSK";
//byte ip[]     = { 10, 0, 0, 150 }; // IP for this device

/*
 * MQTT Settings
 */
//char var = 0;
char* topic = "/radiation";
char state = 0;
byte broker[] = { 10, 0, 0, 3 }; // IP Address of your MQTT Server


/*
 * calculation vars
 */
long count = 0;
long CPM = 0;
long timePrevious = 0;
long timePreviousMeasure = 0;
long countPrevious = 0;

/*
 * Floats
 */
float radiationDose = 0.0;
/*
 * Strings
 */
String clientName;
/*
 * Initialize a counter
 */
//uint8_t counter[1];
// Callback function header
void callback(char* topic, byte* payload, unsigned int length);
WiFiServer server(1883);
WiFiClient wificlient;
PubSubClient client(broker, 1883, callback, wificlient);

//  attachInterrupt(digitalPinToInterrupt(PulsePin), IncreaseHitsPerMinute, RISING);
void setup()
{
  pinMode(PulsePin, INPUT);
  pinMode(LedPin, OUTPUT);
  digitalWrite(PulsePin,HIGH);
  digitalWrite(LedPin,HIGH);
  attachInterrupt(digitalPinToInterrupt(PulsePin),countPulse,FALLING);
  Serial.begin(115200);
//  delay(100);
  WiFiClient wclient = server.available();
  WiFiServer server = wclient.connected();
  InitWiFi();
  InitMQTT(); 
}

void loop()
{

//    if (!client.connected()) {
//    InitWiFi();
//    delay(100);
//  }
  client.loop();
} 

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void InitWiFi()
{
  client.publish("/radiation/log","Starting WiFi connection");
  Serial.println("Starting WiFi connection "); 
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
//  client.publish("/radiation/log","WiFi connected");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  
  server.begin();
  Serial.println("Webserver started");
  client.publish("/radiation/log","Webserver started");
  }

void InitMQTT()
{
  if (client.connect("arduinoClient")) {
    client.publish("/radiation/log","MQTT Connected");
    Serial.println("MQTT Connected");
    client.publish("/radiation/log","SwitchSense online!");
    client.subscribe("/home/out/radiation/#");  // Subscribe to all messages for this device
  }
  Serial.print("Connecting to ");
  client.publish("/radiation/log","Connecting to ");
//  Serial.print(broker);
  client.publish("/radiation/log","as");
  Serial.print(" as ");
  client.publish("/radiation/log","radiation");
  Serial.println(clientName);
  if (client.connect((char*) clientName.c_str())) {
    client.publish("/radiation/log","Connected to MQTT broker");
    Serial.println("Connected to MQTT broker");
    client.publish("/radiation/log","topic is: ");
    Serial.print("topic is: ");
    Serial.println(topic);
    if (client.publish("/radiation/log", "hello from Babylawn!")) {
      Serial.println("Publish ok");
      client.publish("/radiation/log","Publish ok");
    }
    else {
      client.publish("/radiation/log","Publish failed!");
      Serial.println("Publish failed");
    }
  }
  else {
    Serial.println("MQTT connect failed");
    client.publish("/radiation/log","MQTT connect failed");
    Serial.println("Will reset and try again...");
    client.publish("/radiation/log","Will reset and try again...");
    abort();
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  client.publish("/radiation/log","callback");
  Serial.print((String) topic);
  Serial.println(" is topic");
  Serial.print((byte) payload[0]);
  Serial.println(" is payload");
}
String PayloadConstructor() {
  String payload = "{\"radiationDose\":";
  payload += radiationDose;
//  payload += ",\"µSv/h":";
//  payload += uSv;
//  payload += ",\"lastcSv\":";
//  payload += lastcSv;
//  payload += ",\"minHolder\":";
//  payload += minHolder;
//  payload += ",\"minuteHits\":";
//  payload += minuteHits;
  payload += "}";
  Serial.println("payload object: ");
  Serial.println(payload);
}

void ClientConstructor() {
clientName += "ESP-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
}
void countPulse(){
  detachInterrupt(0);
   count++;
  digitalWrite(LedPin,LOW);
  Serial.print("CPM increased: ");
  CountToCPM();
  Serial.println(CPM);
  digitalWrite(LedPin,HIGH);
  while(digitalRead(2)==0){
  } 
}
int CountToCPM(){
      CPM = 6*count;
      CPMtoDose();
    }
void CPMtoDose() {
  if (millis()-timePreviousMeasure > 10000){
    CountToCPM();
    float radiationDose = CPM * CONV_FACTOR;
    timePreviousMeasure = millis();
////    Serial.print("CPM: "); 
////    Serial.println(CPM);
//    Serial.print(" * ");
//    Serial.print(CONV_FACTOR);
//    Serial.print(" = ");
//    Serial.println(radiationDose);      
//    Serial.print("CPM=");
//    Serial.println(CPM);
    Serial.print(radiationDose);
    Serial.println(" uSv/h");
    count = 0;
  }
}
