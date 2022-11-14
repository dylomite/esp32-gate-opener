#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define bleServerName "BLE_SRV_ESP32"
#define SERVICE_UUID "61e9c7f6-28b7-49e8-bf5a-9e0061575c27"
#define CHARACTERISTIC_UUID "cba1d466-344c-4be3-ab3f-189f80dd7518"

unsigned long lastTime = 0;
unsigned long timerDelay = 250;//ms
unsigned long ledBlinkingDelay = 80;//ms
bool deviceConnected = false;
bool isAdvertising = false;

//Define Characteristic and Descriptor
BLECharacteristic bleCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
BLEDescriptor bleDescriptor(BLEUUID((uint16_t)0x2902));

const byte PIN_GPIO_CONN_LED = 2;
const byte PIN_GPIO_CHANNEL_A = 4;
const byte PIN_GPIO_CHANNEL_B = 5;

//Setup BLEServerCallbacks
class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Device Connected!");
    deviceConnected = true;

    isAdvertising = false;
    pServer->getAdvertising()->stop();
    digitalWrite(PIN_GPIO_CONN_LED, HIGH); 
  };
  void onDisconnect(BLEServer* pServer) {
    Serial.println("Device Disonnected!");
    deviceConnected = false;
    digitalWrite(PIN_GPIO_CONN_LED, LOW); 
    pServer->getAdvertising()->start();
    isAdvertising = true;
  }
};

BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;

void setup() {
  // Start serial communication 
  Serial.begin(19200);
  pinMode(PIN_GPIO_CONN_LED, OUTPUT);
  pinMode(PIN_GPIO_CHANNEL_A, OUTPUT);
  pinMode(PIN_GPIO_CHANNEL_B, OUTPUT);
  setupBleServer();
  digitalWrite(PIN_GPIO_CONN_LED, LOW); 
  digitalWrite(PIN_GPIO_CHANNEL_A, LOW);
  digitalWrite(PIN_GPIO_CHANNEL_B, LOW);
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {   
      std::string value = pCharacteristic->getValue();
      onValueReceived(value);
      lastTime = millis();
    }
  }else if(isAdvertising){
    onAdvertising();
  }
}

void setupBleServer() {
  // Create the BLE Device
  BLEDevice::init(bleServerName);
  
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |BLECharacteristic::PROPERTY_WRITE);
  
  pCharacteristic->setValue("YET TO INIT...");
  pService->start();

  setupBleAdvertising();
  
  Serial.println("Characteristic defined! Now you can read it in the Client!");
}

void setupBleAdvertising(){
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);//functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  isAdvertising = true;
}

void onAdvertising(){
  delay(ledBlinkingDelay);
  digitalWrite(PIN_GPIO_CONN_LED, LOW);
  delay(ledBlinkingDelay);
  digitalWrite(PIN_GPIO_CONN_LED, HIGH);
}

void onValueReceived(std::string value){
  String readValue = value.c_str();
  Serial.print("readValue ==> ");
  Serial.println(readValue);

  byte ledvibOutput = LOW;
  if(readValue == "1"){
     ledvibOutput = HIGH;
  }
  digitalWrite(PIN_GPIO_CHANNEL_A, ledvibOutput); 
  
  pCharacteristic->notify();
}
