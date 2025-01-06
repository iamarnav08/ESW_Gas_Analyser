#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <Server.h>
#include <PubSubClient.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <MQ2.h>
#include <SPI.h>
#include <LoRa.h>

// Define the LoRa module pin connections
#define LORA_SCK     18    // GPIO5 - SCK
#define LORA_MISO    19   // GPIO19 - MISO
#define LORA_MOSI    23   // GPIO27 - MOSI
#define LORA_SS      5   // GPIO18 - NSS
#define LORA_RST     14  // GPIO14 - RESET
#define LORA_DIO0    2   // GPIO26 - DIO0

// Define the frequency for LoRa communication (433 MHz, 868 MHz, or 915 MHz)
#define LORA_BAND    433E6  // 433 MHz (Adjust according to your region)

const char* ssid = "Hiten17";
const char* pass = "qwerty12";

// Ensure that the credentials here allow you to publish and subscribe to the ThingSpeak channel.
const long channelID = 2687973;  // Replace 123456 with your actual ThingSpeak channel number 
const char mqttUserName[] = "BQMWHSM1Ny4lCzkRMDsQLRQ";
const char clientID[] = "BQMWHSM1Ny4lCzkRMDsQLRQ";
const char mqttPass[] = "zFipNYCiHte2pLUVoKiKgm6o";

#define mqttPort 1883
WiFiClient client;
const char* server = "mqtt3.thingspeak.com";
int status = WL_IDLE_STATUS;
long lastPublishMillis = 0;
int connectionDelay = 1;
int updateInterval = 5;
int rssi;
int ssr;

PubSubClient mqttClient(client);

float lpg, co, smoke, packetNumber;
String incomingMessage;

void setup() {
  // Initialize the Serial Monitor for debugging
  Serial.begin(115200);
  while (!Serial);

  // Setup LoRa pins
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  // Start LoRa module with the defined frequency
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialized successfully!");
  connectWifi();
  mqttClient.setServer(server, mqttPort);

  delay(3000);
}

void loop() {
  // Check if there's an incoming packet
  // how to recieve multiple packets ?

  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
  if (!mqttClient.connected()) {
    mqttConnect();
    mqttSubscribe(channelID);
  }
  mqttClient.loop();
  delay(100);


  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read packet and print its content
    Serial.print("Received packet: ");

    // Read the incoming message
    while (LoRa.available()) {
      incomingMessage = LoRa.readString();
      // Serial.print(incomingMessage);
    }
    // Serial.println();
    Serial.println(incomingMessage);
    Serial.println("-----------------------------");
    rssi = LoRa.packetRssi();  // Get RSSI value
    Serial.print("Signal strength (RSSI): ");
    Serial.println(rssi);          // Print RSSI value
    ssr = LoRa.packetSnr();    // Get SNR value // Signal to noise ratio 
    Serial.print("Signal to noise ratio (SNR): ");
    Serial.println(ssr);           // Print SNR value
    Serial.println("-----------------------------");
  }

   if (incomingMessage.length() > 0) {
      // Assume the incoming message is in the format: "lpg,co,smoke"
      int commaIndex1 = incomingMessage.indexOf(' ');
      int commaIndex2 = incomingMessage.indexOf(' ', commaIndex1 + 1);
      int commaIndex3 = incomingMessage.indexOf(' ',commaIndex2 + 1);
  
    if (commaIndex1 != -1 && commaIndex2 != -1 && commaIndex3 != -1) {
        // Extract LPG, CO, and smoke values from the message
        String lpgString = incomingMessage.substring(0, commaIndex1);
        String coString = incomingMessage.substring(commaIndex1 + 1, commaIndex2);
        String smokeString = incomingMessage.substring(commaIndex2 + 1, commaIndex3);
        String PacketString = incomingMessage.substring(commaIndex3 + 1);

        // Convert the strings to float values
        lpg = lpgString.toFloat();
        co = coString.toFloat();
        smoke = smokeString.toFloat();
        packetNumber = PacketString.toFloat();

        // Print the separated values for debugging
        // Serial.print("LPG: ");
        // Serial.println(lpg);
        // Serial.print("CO: ");
        // Serial.println(co);
        // Serial.print("Smoke: ");
        // Serial.println(smoke);
        if (abs(long(millis()) - lastPublishMillis) > updateInterval * 1000) {
          String topicString = "channels/" + String(channelID) + "/publish";

          String message = "&field1=" + String(co);
          message += "&field2=" + String(smoke);
          message += "&field3=" + String(lpg);
          message += "&field4=" + String(rssi);
          message += "&field5=" + String(ssr);
          message += "&field6=" + String(packetNumber);
          mqttClient.publish(topicString.c_str(), message.c_str());
          lastPublishMillis = millis();
        }
    } else {
      // Serial.println("Error: Invalid packet format.");
    }
  }


}
void mqttConnect() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(clientID, mqttUserName, mqttPass)) {
      Serial.println("MQTT connected.");
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(mqttClient.state());
      // delay(5000);
    }
  }
}
void connectWifi() {
  Serial.println("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, pass);
    delay(1000);
    Serial.println(".");
  }
  Serial.println("Connected to Wi-Fi");
}
void mqttSubscribe(long subChannelID) {
  String myTopic = "channels/" + String(subChannelID) + "/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}
