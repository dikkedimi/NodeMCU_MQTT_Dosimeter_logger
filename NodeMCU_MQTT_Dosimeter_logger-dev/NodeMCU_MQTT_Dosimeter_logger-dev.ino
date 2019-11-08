#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Thread.h>
#include <StaticThreadController.h>

#define tubeIndex 151
#define PulsePin 13 //
#define LedPin 16 // 

#define LOG_PERIOD 10
#define MAX_PERIOD 60
#define ENTRIES (MAX_PERIOD / LOG_PERIOD)

const char *ssid =  "SSID";
const char *pass =  "PSK";

char* topic = "/radiation";
char state = 0;
byte broker[] = { 10, 0, 0, 3 };
String clientName;
void callback(char* topic, byte* payload, unsigned int length);
unsigned long counts[2] = {0, 0}, logs[ENTRIES];

WiFiServer server(1883);
WiFiClient wificlient;
PubSubClient client(broker, 1883, callback, wificlient);

Thread threadCurrentLog = Thread();
StaticThreadController<1> threadController (&threadCurrentLog);

void setup()
{
  for (int _i = 0; _i < ENTRIES; _i++)
    logs[_i] = 0;
  
  Serial.begin(115200);

  pinMode(PulsePin, INPUT);
  pinMode(LedPin, OUTPUT);
  digitalWrite(LedPin,LOW);

  WiFiClient wclient = server.available();
  WiFiServer server = wclient.connected();
  ClientConstructor();
  InitWiFi();
  InitMQTT();

  threadCurrentLog.enabled = true;
  threadCurrentLog.setInterval(LOG_PERIOD * 1000);
  threadCurrentLog.onRun(threadCurrentLogCallback);

  attachInterrupt(digitalPinToInterrupt(PulsePin), countPulse, FALLING);
}


void loop()
{
  threadController.run();
  client.loop();
} 


String macToStr(const uint8_t* mac)
{
  String result;
  
  for (int i = 0; i < 6; ++i)
  {
    result += String(mac[i], 16);
    
    if (i < 5) result += ':';
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
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  
  client.publish("/radiation/log","WiFi connected");
  
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
  if (client.connect("arduinoClient"))
  {
    client.publish("/radiation/log","MQTT Connected");
    Serial.println("MQTT Connected");
    client.publish("/radiation/log","RadSense online!");
    client.subscribe("/home/out/radiation/#");  // Subscribe to all messages for this device
  }
  
  Serial.print("Connecting to ");
  client.publish("/radiation/log","Connecting to ");

  Serial.print(" as ");
  client.publish("/radiation/log","as");
  
  Serial.println(clientName);
  client.publish("/radiation/log","radiation");
  
  if (client.connect((char*) clientName.c_str()))
  {
    Serial.println("Connected to MQTT broker");
    client.publish("/radiation/log","Connected to MQTT broker");
    
    Serial.print("topic is: ");
    Serial.println(topic);
    client.publish("/radiation/log","topic is: ");
    
    
    if (client.publish("/radiation/log", "hello from Babylawn!"))
    {
      Serial.println("Publish ok");
      client.publish("/radiation/log","Publish ok");
      return;
    }
    else
    {
      Serial.println("Publish failed");
//      client.publish("/radiation/log","Publish failed!");
    }
  }
  else
  {
    Serial.println("MQTT connect failed");
//    client.publish("/radiation/log","MQTT connect failed");
    Serial.println("Will reset and try again...");
//    client.publish("/radiation/log","Will reset and try again...");
    
    return;
  }
}


String PayloadConstructor()
{
  float _cpmTotal = (float)counts[1] / float (millis() / 1000) * 60; // dit is een inter/extrapolatie van het totaal
  float _cpmMinuteAverage = 0; // dit is het average van de afgelopen minuut
  float _dose = _cpmTotal / tubeIndex; // dit is de tube index CPM / 151 = uSv/h
  
  for (int _i = ENTRIES - 1; _i > 0; _i--)
    _cpmMinuteAverage += logs[_i];

  String payload = "{\"CPMTotal\":";
         payload += (String)_cpmTotal;
         payload += ",\"cpmMinuteAverage\":";
         payload += (String)_cpmMinuteAverage;
         payload += ",\"dose\":";
         payload += (String)_dose;
         payload += "}";
//         Serial.println(_dose);
         Serial.println(payload);
         if(client.publish(topic, (char*) payload.c_str())) {
          Serial.println(payload);
          Serial.println("Publish OK!");
         } else {
          Serial.println("Publish FAIL!");
          InitMQTT();
         }
}


void ClientConstructor()
{
  clientName = "ESP-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
}


void callback(char* topic, byte* payload, unsigned int length)
{
  client.publish("/radiation/log","callback");
  Serial.print((String) topic);
  Serial.println(" is topic");
  Serial.print((byte) payload[0]);
  Serial.println(" is payload");

}


void countPulse()
{
  digitalWrite(LedPin,LOW);
  counts[0]++;
  digitalWrite(LedPin,HIGH);
}


void threadCurrentLogCallback()
{
  for (int _i = ENTRIES - 1; _i > 0; _i--)
    logs[_i] = logs[_i - 1];

  logs[0] = counts[0];
  counts[1] += counts[0];
  counts[0] = 0;
  PayloadConstructor();
}
