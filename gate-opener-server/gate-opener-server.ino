#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define PIN_GPIO_CONN_LED 2
#define PIN_GPIO_CHANNEL_A 4
#define PIN_GPIO_CHANNEL_B 18
#define SERVER_NAME "ESP32 Gate Opener BtServer"
#define SERVICE_UUID "bc5d00ca-badf-4244-ae11-4a8f73fd409f"
#define CHARACTERISTIC_UUID_CHANNEL_A "1dd23185-5d48-457c-a5e4-0f21afed2e58"
#define CHARACTERISTIC_UUID_CHANNEL_B "04889dda-9528-45fb-ac11-f522997afb0b"
#define SIG_OFF "SIG_OFF"
#define SIG_ON "SIG_ON"
#define DELTA_TIME_MS_CONN_LED_BLINK 80
#define DELTA_TIME_MS_READ_VALUES 100
#define MAX_HIGH_FOR_MS_VALUE 1500


unsigned long lastTimeReadMs = 0;
bool deviceConnected = false;
bool isAdvertising = false;

BLECharacteristic* pCharacteristicA;
BLECharacteristic* pCharacteristicB;

void setup() {
    Serial.begin(19200);
    pinMode(PIN_GPIO_CONN_LED, OUTPUT);
    pinMode(PIN_GPIO_CHANNEL_A, OUTPUT);
    pinMode(PIN_GPIO_CHANNEL_B, OUTPUT);

    lastTimeReadMs = 0;
    deviceConnected = false;
    isAdvertising = false;

    setupBleServer();
    digitalWrite(PIN_GPIO_CONN_LED, LOW);
    initOutputValueAndNotify(LOW, LOW);
}

unsigned long aHighSince = 0;//TODO: rm unsigned
unsigned long bHighSince = 0;//TODO: impl


void loop() {
    if (deviceConnected) {
        if ((millis() - lastTimeReadMs) > DELTA_TIME_MS_READ_VALUES) {
            byte outputA = readCharacteristicValue(pCharacteristicA);
            byte outputB = readCharacteristicValue(pCharacteristicB);
            Serial.print("outputA = ");
            Serial.print(outputA);
            Serial.print(" outputB = ");
            Serial.println(outputB);
            //TODO: ALSO IMPL B

            setupHighSinceDeltaT(outputA,&aHighSince);
            setupHighSinceDeltaT(outputB,&bHighSince);

            //CHANNEL A
            if(aHighSince>MAX_HIGH_FOR_MS_VALUE){
              onForceLowSig(pCharacteristicA, &outputA, &aHighSince);
            }
            onSetOutput(pCharacteristicA,PIN_GPIO_CHANNEL_A,outputA);

            //CHANNEL B
            if(bHighSince>MAX_HIGH_FOR_MS_VALUE){
              onForceLowSig(pCharacteristicB, &outputB, &bHighSince);
            }
            onSetOutput(pCharacteristicB,PIN_GPIO_CHANNEL_B,outputB);

            lastTimeReadMs = millis();
        }
    } else if (isAdvertising) {
        onAdvertising();
    }
}

void setupHighSinceDeltaT(byte outputA, unsigned long *highSince){
  if (outputA == HIGH) {
      *highSince += DELTA_TIME_MS_READ_VALUES;
  } else {
      *highSince = 0;
  }
}

void onForceLowSig(BLECharacteristic* characteristic,byte *output,unsigned long *highSince){
    characteristic->setValue(SIG_OFF);
    output = LOW;
    highSince = 0;
}

byte readCharacteristicValue(BLECharacteristic* characteristic) {
    std::string readValue = characteristic->getValue();
    String valueStr = readValue.c_str();
    byte output = getPinOutputValue(valueStr);
    return output;
}

byte getPinOutputValue(String characteristicValue) {
    return characteristicValue == SIG_ON ? HIGH : LOW;
}

void onSetOutput(BLECharacteristic* characteristic,int pinGpio,byte output){
    digitalWrite(pinGpio, output);
    characteristic->notify();
}

//TODO: rm this!
void initOutputValueAndNotify(byte outputB, byte outputA) {
  onSetOutput(pCharacteristicA,PIN_GPIO_CHANNEL_A,outputA);
  onSetOutput(pCharacteristicB,PIN_GPIO_CHANNEL_B,outputB);
}

void setupBleAdvertising() {
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    isAdvertising = true;
}

void onAdvertising() {
    delay(DELTA_TIME_MS_CONN_LED_BLINK);
    digitalWrite(PIN_GPIO_CONN_LED, LOW);
    delay(DELTA_TIME_MS_CONN_LED_BLINK);
    digitalWrite(PIN_GPIO_CONN_LED, HIGH);
}

// Setup BLEServerCallbacks
class ServerCallbacks : public BLEServerCallbacks {
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
        initOutputValueAndNotify(LOW, LOW);
        pServer->getAdvertising()->start();
        isAdvertising = true;
    }
};

void setupBleServer() {
    BLEDevice::init(SERVER_NAME);

    BLEServer* pServer;
    BLEService* pService;

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);
    pCharacteristicA = pService->createCharacteristic(CHARACTERISTIC_UUID_CHANNEL_A, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicA->setValue(SIG_OFF);
    pCharacteristicB = pService->createCharacteristic(CHARACTERISTIC_UUID_CHANNEL_B, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicB->setValue(SIG_OFF);
    pService->start();

    setupBleAdvertising();
}
