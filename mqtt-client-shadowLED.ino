#include <WioLTEforArduino.h>
#include <WioLTEClient.h>
#include <PubSubClient.h>		// https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>
#include <stdio.h>

#define APN               "soracom.io"
#define USERNAME          "sora"
#define PASSWORD          "sora"

#define MQTT_SERVER_HOST  "beam.soracom.io"
#define MQTT_SERVER_PORT  (1883)

#define ID                "<device name>"
#define OUT_TOPIC         "$aws/things/tkizawa-WioLTE/shadow/update"
#define IN_TOPIC          "$aws/things/tkizawa-WioLTE/shadow/update/delta"

WioLTE Wio;
WioLTEClient WioClient(&Wio);
PubSubClient MqttClient;

void callback(char* topic, byte* payload, unsigned int length) {
  char subsc[length];
  for (int i = 0; i < length; i++) subsc[i]=(char)payload[i];
  subsc[length]='\0';
  SerialUSB.println("### Subscribe");
  SerialUSB.println(subsc);

  // JSON parse
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(subsc);

  if (!root.success()) {
    SerialUSB.println("parseObject() failed");
  }
  else{
    // Change LED color
    const char* ledcolor = root["state"]["LED"];
    SerialUSB.print("Change LED color to: ");
    SerialUSB.println(ledcolor);

    int r = 0;
    int g = 0;
    int b = 0;

    if      (strcmp(ledcolor,"red") == 0)   { r = 255; }
    else if (strcmp(ledcolor,"green") == 0) { g = 255; }
    else if (strcmp(ledcolor,"blue") == 0)  { b = 255; }
    else if (strcmp(ledcolor,"yellow") == 0){ r = 255; g = 255; }
    else if (strcmp(ledcolor,"purple") == 0){ r = 255; b = 255; }
    else if (strcmp(ledcolor,"lblue") == 0) { g = 255; b = 255; }
    else if (strcmp(ledcolor,"white") == 0) { r = 255; g = 255; b = 255; }
    else if (strcmp(ledcolor,"off") == 0)   {          }
    else {
      SerialUSB.println("Incollect color ID");
      return;
    }
    Wio.LedSetRGB(r,g,b);
    SerialUSB.println("### LED Status Sent.");  

    // RePublish LED Status
    char data[100];
    sprintf(data,"{\"state\": {\"reported\" : {\"LED\" : \"%s\"}}}",ledcolor);
    MqttClient.publish(OUT_TOPIC, data);
  }
}

void setup() {
  delay(200);

  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");
  
  SerialUSB.println("### I/O Initialize.");
  Wio.Init();
  
  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyLTE(true);
  delay(500);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Connecting to \""APN"\".");
  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }

  SerialUSB.println("### Connecting to MQTT server \""MQTT_SERVER_HOST"\"");
  MqttClient.setServer(MQTT_SERVER_HOST, MQTT_SERVER_PORT);
  MqttClient.setCallback(callback);
  MqttClient.setClient(WioClient);
  if (!MqttClient.connect(ID)) {
    SerialUSB.println("### ERROR! ###");
    return;
  }
  int qos=0;
  MqttClient.subscribe(IN_TOPIC,qos);
  SerialUSB.println("### Setup completed.");

  // Send Initialize LED Status
  char *data = "{\"state\": {\"reported\" : {\"LED\" : \"off\"}}}";
  MqttClient.publish(OUT_TOPIC, data);
  SerialUSB.println("### LED Status Sent.");  
}

void loop() {
  MqttClient.loop();
}

