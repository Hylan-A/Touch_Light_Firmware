#include "OptionalMqtt.h"
#include "WebControl.h"
#include "Diagnostics.h"

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Preferences.h>

// MQTT is optional, local-network only, and disabled by default.
// These credentials belong to the homeowner; no passwords are in source.
namespace {
WiFiClient mqttTransport;
PubSubClient client(mqttTransport);

bool enabled=false;
bool lastPower=false;
uint8_t lastBrightness=180;
bool haveState=false;
String brokerHost, brokerUsername, brokerPassword;
uint16_t brokerPort=1883;
String unitSuffix, clientId;
String commandPower, commandBrightness, statePower, stateBrightness;
String topicAvailability, topicDiscovery;
unsigned long lastAttempt=0;
constexpr unsigned long MQTT_RETRY_MS=12000;

void publishState()
{
  if(!client.connected() || !haveState)return;
  client.publish(statePower.c_str(),lastPower?"ON":"OFF",true);
  char level[5];
  snprintf(level,sizeof(level),"%u",lastBrightness);
  client.publish(stateBrightness.c_str(),level,true);
}

void publishDiscovery()
{
  // Per-chip unique discovery IDs avoid collisions between gifted lights.
  String body;
  body.reserve(950);
  body+="{\"name\":\"Tech 49 Touch Light ";
  body+=unitSuffix;
  body+="\",\"unique_id\":\"tech49_light_";
  body+=unitSuffix;
  body+="\",\"command_topic\":\"";
  body+=commandPower;
  body+="\",\"state_topic\":\"";
  body+=statePower;
  body+="\",\"brightness_command_topic\":\"";
  body+=commandBrightness;
  body+="\",\"brightness_state_topic\":\"";
  body+=stateBrightness;
  body+="\",\"brightness_scale\":255,\"payload_on\":\"ON\",\"payload_off\":\"OFF\",";
  body+="\"availability_topic\":\"";
  body+=topicAvailability;
  body+="\",\"payload_available\":\"online\",\"payload_not_available\":\"offline\",";
  body+="\"device\":{\"identifiers\":[\"tech49_light_";
  body+=unitSuffix;
  body+="\"],\"name\":\"Tech 49 Touch Light ";
  body+=unitSuffix;
  body+="\",\"manufacturer\":\"Tech 49 Lab\",\"model\":\"ESP32-C3 Touch Light 0008\"}}";
  if(!client.publish(topicDiscovery.c_str(),body.c_str(),true)) {
    Serial.println("MQTT: Home Assistant discovery publish failed.");
    diagLog("[MQTT] Home Assistant discovery publish failed");
  } else {
    Serial.println("MQTT: Home Assistant discovery published.");
    diagLog("[MQTT] Home Assistant discovery published");
  }
}

void callback(char* topic, byte* payload, unsigned int length)
{
  String value;
  value.reserve(length);
  for(unsigned int i=0;i<length;i++)value+=static_cast<char>(payload[i]);
  value.trim();
  if(commandPower==topic) {
    if(value.equalsIgnoreCase("ON"))
      mqttControlCommand(true,true,false,0);
    else if(value.equalsIgnoreCase("OFF"))
      mqttControlCommand(true,false,false,0);
  } else if(commandBrightness==topic) {
    if(value.length()==0 || value.length()>3)return;
    for(size_t i=0;i<value.length();i++)
      if(value[i]<'0'||value[i]>'9')return;
    const int level=value.toInt();
    if(level>=0 && level<=255)
      mqttControlCommand(false,false,true,static_cast<uint8_t>(level));
  }
}
} // namespace

bool optionalMqttSaveSettings(bool active,
                              const String &host, uint16_t port,
                              const String &username, const String &password)
{
  Preferences prefs;
  if(!prefs.begin("tech49mqtt",false))return false;
  // Erase stale broker credentials when Home Assistant is disabled.
  prefs.putBool("enabled",active);
  if(active){
    prefs.putString("host",host);
    prefs.putUShort("port",port);
    prefs.putString("user",username);
    prefs.putString("pass",password);
  }else{
    prefs.remove("host");
    prefs.remove("port");
    prefs.remove("user");
    prefs.remove("pass");
  }
  prefs.end();
  return true;
}

void optionalMqttEraseSettings()
{
  Preferences prefs;
  if(prefs.begin("tech49mqtt",false)){
    prefs.clear();
    prefs.end();
  }
}

bool optionalMqttEnabled() {return enabled;}
bool optionalMqttConnected() {return enabled && client.connected();}

void optionalMqttBegin(const String &suffix)
{
  unitSuffix=suffix;
  unitSuffix.toLowerCase();
  clientId="tech49-light-"+unitSuffix;
  const String prefix="tech49/light/"+unitSuffix+"/";
  commandPower=prefix+"cmd/power";
  commandBrightness=prefix+"cmd/brightness";
  statePower=prefix+"state/power";
  stateBrightness=prefix+"state/brightness";
  topicAvailability=prefix+"status";
  topicDiscovery="homeassistant/light/tech49_light_"+unitSuffix+"/config";

  Preferences prefs;
  if(prefs.begin("tech49mqtt",true)){
    enabled=prefs.getBool("enabled",false);
    brokerHost=prefs.getString("host","");
    brokerPort=prefs.getUShort("port",1883);
    brokerUsername=prefs.getString("user","");
    brokerPassword=prefs.getString("pass","");
    prefs.end();
  }
  if(brokerHost.isEmpty() || brokerPort==0)enabled=false;

  client.setBufferSize(1024);
  client.setSocketTimeout(2);
  client.setCallback(callback);
  if(enabled) {
    client.setServer(brokerHost.c_str(),brokerPort);
    Serial.println("Optional Home Assistant MQTT: enabled.");
    diagLog("[MQTT] Home Assistant enabled");
  }else{
    Serial.println("Optional Home Assistant MQTT: disabled.");
    diagLog("[MQTT] Home Assistant disabled");
  }
  lastAttempt=millis()-MQTT_RETRY_MS;
}

void optionalMqttPublishState(bool on, uint8_t brightness)
{
  lastPower=on;
  lastBrightness=brightness;
  haveState=true;
  publishState();
}

void optionalMqttLoop()
{
  if(!enabled || WiFi.status()!=WL_CONNECTED)return;
  if(client.connected()){
    client.loop();
    return;
  }
  const unsigned long now=millis();
  if(now-lastAttempt<MQTT_RETRY_MS)return;
  lastAttempt=now;
  Serial.println("MQTT: connecting to configured broker...");
  diagLog("[MQTT] Connecting to broker");
  bool connected=false;
  if(brokerUsername.isEmpty()){
    connected=client.connect(clientId.c_str(),
                             topicAvailability.c_str(),0,true,"offline");
  }else{
    connected=client.connect(clientId.c_str(),brokerUsername.c_str(),
                             brokerPassword.c_str(),topicAvailability.c_str(),
                             0,true,"offline");
  }
  if(!connected){
    Serial.print("MQTT: connection failed, code = ");
    Serial.println(client.state());
    diagLogf("[MQTT] Connection failed, code %d",client.state());
    return;
  }
  Serial.println("MQTT: connected.");
  diagLog("[MQTT] Connected");
  client.publish(topicAvailability.c_str(),"online",true);
  client.subscribe(commandPower.c_str());
  client.subscribe(commandBrightness.c_str());
  publishDiscovery();
  publishState();
}


String optionalMqttBrokerHost(){return brokerHost;}
