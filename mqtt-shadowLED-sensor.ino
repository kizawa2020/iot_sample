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
#define OUT_TOPIC         "$aws/things/(device name)/shadow/update"
#define IN_TOPIC          "$aws/things/(device name)/shadow/update/delta"
#define SENSOR_TOPIC      "sensordata/"

#define INTERVAL          (60000)
#define RECEIVE_TIMEOUT   (10000)

#define SENSOR_PIN        (WIOLTE_D38)

int TemperatureAndHumidityPin;

WioLTE Wio;
WioLTEClient WioClient(&Wio);
PubSubClient MqttClient;

void callback(char* topic, byte* payload, unsigned int length) {
  SerialUSB.println("### Subscribe");
  char subsc[length];
  for (int i = 0; i < length; i++) subsc[i]=(char)payload[i];
  subsc[length]='\0';
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
    SerialUSB.print("Change LED color to: ");
    SerialUSB.println(ledcolor);
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

  // Sensor Initilize
  TemperatureAndHumidityBegin(SENSOR_PIN);
  SerialUSB.println("### Sensor Initialized.");
  SerialUSB.println("### Setup completed.");

  // Send Initialize LED Status
  char *data = "{\"state\": {\"reported\" : {\"LED\" : \"off\"}}}";
  MqttClient.publish(OUT_TOPIC, data);
  SerialUSB.println("### LED Status Sent.");  
}

void loop() {
  char data[100];

  float temp;
  float humi;

  SerialUSB.println("### Read Temperature & Humidity from Sensor.");  
  if (!TemperatureAndHumidityRead(&temp, &humi)) {
    SerialUSB.println("ERROR!");
  }
  else{
    SerialUSB.print("temperature = ");
    SerialUSB.print(temp);
    SerialUSB.print("C  ");
    SerialUSB.print("humidity = ");
    SerialUSB.print(humi);
    SerialUSB.println("%  ");

    SerialUSB.println("### SensorData Send.");
    sprintf(data,"{\"temp\":%.1f,\"humi\":%.1f}", temp, humi);
    MqttClient.publish(SENSOR_TOPIC, data);
  }

  unsigned long next = millis();
  while (millis() < next + INTERVAL)
  {
    MqttClient.loop();
  }
}

void TemperatureAndHumidityBegin(int pin)
{
  TemperatureAndHumidityPin = pin;
  DHT11Init(TemperatureAndHumidityPin);
}

bool TemperatureAndHumidityRead(float* temperature, float* humidity)
{
  byte data[5];

  DHT11Start(TemperatureAndHumidityPin);
  for (int i = 0; i < 5; i++) data[i] = DHT11ReadByte(TemperatureAndHumidityPin);
  DHT11Finish(TemperatureAndHumidityPin);

  if(!DHT11Check(data, sizeof (data))) return false;
  if (data[1] >= 10) return false;
  if (data[3] >= 10) return false;

  *humidity = (float)data[0] + (float)data[1] / 10.0f;
  *temperature = (float)data[2] + (float)data[3] / 10.0f;

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////
//


void DHT11Init(int pin)
{
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

void DHT11Start(int pin)
{
  // Host the start of signal
  digitalWrite(pin, LOW);
  delay(18);

  // Pulled up to wait for
  pinMode(pin, INPUT);
  while (!digitalRead(pin)) ;

  // Response signal
  while (digitalRead(pin)) ;

  // Pulled ready to output
  while (!digitalRead(pin)) ;
}

byte DHT11ReadByte(int pin)
{
  byte data = 0;

  for (int i = 0; i < 8; i++) {
    while (digitalRead(pin)) ;

    while (!digitalRead(pin)) ;
    unsigned long start = micros();

    while (digitalRead(pin)) ;
    unsigned long finish = micros();

    if ((unsigned long)(finish - start) > 50) data |= 1 << (7 - i);
  }
  return data;
}

void DHT11Finish(int pin)
{
  // Releases the bus
  while (!digitalRead(pin)) ;
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

bool DHT11Check(const byte* data, int dataSize)
{
  if (dataSize != 5) return false;

  byte sum = 0;
  for (int i = 0; i < dataSize - 1; i++) {
    sum += data[i];
  }

  return data[dataSize - 1] == sum;
}

