#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include <MQ2.h>
#include <LiquidCrystal.h>

// Define LoRa and SD card pin connections
#define SCK     18    // GPIO5 - SCK
#define MISO    19   // GPIO19 - MISO
#define MOSI    23   // GPIO27 - MOSI
#define LORA_SS  5   // Chip select for LoRa
#define SD_SS    4   // Chip select for SD card (choose a different pin for SD card)
#define LORA_RST 14  // RESET for LoRa
#define LORA_DIO0 2   // DIO0 for LoRa

// LCD Display

// D4 -> GPIO 32
// D5 -> GPIO 33
// D6 -> GPIO 26
// D7 -> GPIO 27
// VCC -> 3.3V
// GND -> GND
// Initialize the library with the interface pins
const int rs = 22, en = 21, d4 = 32, d5 = 33, d6 = 26, d7 = 27;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int packet_count = 0;
int pin = 25;
float lpg, co, smoke;

int updateinterval = 5000;
unsigned long lasttime = 0;
int casel = 0;
bool normal = true;

MQ2 mq2(pin);

// LoRa frequency
#define LORA_BAND 433E6

File myFile;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize SPI bus for both LoRa and SD card
  SPI.begin(SCK, MISO, MOSI);

  pinMode(SD_SS, OUTPUT);
  pinMode(LORA_SS, OUTPUT);

  // Initialize SD card
  digitalWrite(SD_SS, LOW);
  digitalWrite(LORA_SS, HIGH);
  Serial.println("Initializing SD card...");
  while (!SD.begin(SD_SS, SPI)) {
    Serial.println("SD card initialization failed!");
  }
  Serial.println("SD card initialized.");
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  WriteToSD("LPG,CO,Smoke\n");

  digitalWrite(SD_SS, HIGH);  // Disable SD card
  delay(1000);
  // Initialize LoRa
  digitalWrite(LORA_SS, LOW);  // Disable SD card
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_BAND)) {
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  digitalWrite(LORA_SS, HIGH);  // Disable LoRa
  Serial.println("LoRa initialized.");

  delay(1000);
  // Initialize Sensor
  Serial.println("Calibration Started!");
  mq2.begin();
  delay(1000);
  Serial.println("Calibration done!");

  lcd.begin(16, 2);
  delay(1000);
  lcd.clear();
}

void loop() {
  // Read sensor data
  lpg = mq2.readLPG();
  co = mq2.readCO();
  smoke = mq2.readSmoke();
  if (lpg > 1000 || co > 200 || smoke > 400 || isnan(lpg) || isnan(co) || isnan(smoke)) {
    normal = false;
  } else{
    normal = true;
  }

  // Send data over LoRa
  sendLoRaMessage();

  // Save data to SD card
  saveToSD();

  if(millis() - lasttime > updateinterval){
    LCDdisplay();
    lasttime = millis();
  }

  delay(2000);  // Wait before next iteration
}

void sendLoRaMessage() {
  // Disable SD card, enable LoRa
  digitalWrite(SD_SS, HIGH);
  digitalWrite(LORA_SS, LOW);

  // Send LoRa packet
  LoRa.beginPacket();
  LoRa.print(lpg);
  LoRa.print(" ");
  LoRa.print(co);
  LoRa.print(" ");
  LoRa.print(smoke);
  LoRa.print(" ");
  LoRa.print(++packet_count);

  LoRa.endPacket();

  // Disable LoRa after sending
  digitalWrite(LORA_SS, HIGH);
}

void saveToSD() {
  // Disable LoRa, enable SD card
  digitalWrite(LORA_SS, HIGH);
  digitalWrite(SD_SS, LOW);

  // Save data to SD card
  String message = String(lpg) + "," + String(co) + "," + String(smoke) + "\n";
  Serial.print(message);
  Serial.print(packet_count);
  Serial.println("-----------------------------------------");
  WriteToSD(message.c_str()); 

  // Disable SD card after writing
  digitalWrite(SD_SS, HIGH);
}

void WriteToSD(const char *message) {
  myFile = SD.open("/data.csv", FILE_APPEND);
  if (myFile) {
    myFile.print(message);
    myFile.close();
  } else {
    Serial.println("Error opening file on SD card.");
  }
}

void LCDdisplay(){
  lcd.clear();
  switch(casel){
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("CO : ");
      lcd.print(co);
      lcd.setCursor(0, 1);
      if(normal){
        lcd.print("Safe Level");
      } else{
        lcd.print("Danger! Danger!");
      }
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("LPG : ");
      lcd.print(lpg);
      lcd.setCursor(0, 1);
      if(normal){
        lcd.print("Safe Level");
      } else{
        lcd.print("Danger! Danger!");
      }
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Smoke : ");
      lcd.print(smoke);
      lcd.setCursor(0, 1);
      if(normal){
        lcd.print("Safe Level");
      } else{
        lcd.print("Danger! Danger!");
      }
      break;
  }
  casel+=1;
  casel%=3;
}