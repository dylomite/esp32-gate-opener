#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// https://github.com/espressif/arduino-esp32/issues/6329

#define bleServerName "ESP32 Gate Opener BtServer"
#define SERVICE_UUID "bc5d00ca-badf-4244-ae11-4a8f73fd409f"
#define CHARACTERISTIC_UUID_CHANNEL_A "1dd23185-5d48-457c-a5e4-0f21afed2e58"
#define CHARACTERISTIC_UUID_CHANNEL_B "04889dda-9528-45fb-ac11-f522997afb0b"
#define CHARACTERISTIC_VALUE_LOW "0"
#define CHARACTERISTIC_VALUE_HIGH "1"

unsigned long lastTimeRead = 0;       // ms
unsigned long readValuesDelta = 250;  // ms
unsigned long ledBlinkingDelay = 80;  // ms
bool deviceConnected = false;
bool isAdvertising = false;

const byte PIN_GPIO_CONN_LED = 2;
const byte PIN_GPIO_CHANNEL_A = 4;
const byte PIN_GPIO_CHANNEL_B = 5;

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
        pServer->getAdvertising()->start();
        isAdvertising = true;
    }
};

BLECharacteristic* pCharacteristicA;
BLECharacteristic* pCharacteristicB;

void setup() {
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
        if ((millis() - lastTimeRead) > readValuesDelta) {
            std::string valueA = pCharacteristicA->getValue();
            std::string valueB = pCharacteristicB->getValue();
            onReadValues(valueA, valueB);
            lastTimeRead = millis();
        }
    } else if (isAdvertising) {
        onAdvertising();
    }
}

void setupBleServer() {
    BLEDevice::init(bleServerName);

    BLEServer* pServer;
    BLEService* pService;

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);
    pCharacteristicA = pService->createCharacteristic(CHARACTERISTIC_UUID_CHANNEL_A, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicA->setValue(CHARACTERISTIC_VALUE_LOW);
    pCharacteristicB = pService->createCharacteristic(CHARACTERISTIC_UUID_CHANNEL_B, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicB->setValue(CHARACTERISTIC_VALUE_LOW);
    pService->start();

    setupBleAdvertising();
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
    delay(ledBlinkingDelay);
    digitalWrite(PIN_GPIO_CONN_LED, LOW);
    delay(ledBlinkingDelay);
    digitalWrite(PIN_GPIO_CONN_LED, HIGH);
}

// TODO: avoid str convert!
void onReadValues(std::string valueA, std::string valueB) {
    String valueAStr = valueA.c_str();
    String valueBStr = valueB.c_str();

    Serial.print("valueA = ");
    Serial.print(valueAStr);
    Serial.print(" valueB = ");
    Serial.println(valueBStr);

    byte outputA = getPinOutput(valueAStr);
    byte outputB = getPinOutput(valueBStr);

    digitalWrite(PIN_GPIO_CHANNEL_A, outputA);
    pCharacteristicA->notify();
    digitalWrite(PIN_GPIO_CHANNEL_B, outputB);
    pCharacteristicB->notify();
}

byte getPinOutput(String characteristicValue) {
    byte output = LOW;
    if (characteristicValue == CHARACTERISTIC_VALUE_HIGH) {
        output = HIGH;
    }
    return output;
}