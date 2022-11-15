#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// https://github.com/espressif/arduino-esp32/issues/6329

#define bleServerName "ESP32 Gate Opener BtServer"
#define SERVICE_UUID "bc5d00ca-badf-4244-ae11-4a8f73fd409f"
#define CHARACTERISTIC_UUID_CHANNEL_A "1dd23185-5d48-457c-a5e4-0f21afed2e58"
#define CHARACTERISTIC_UUID_CHANNEL_B "04889dda-9528-45fb-ac11-f522997afb0b"

unsigned long lastTime = 0;
unsigned long timerDelay = 250;       // ms
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

BLECharacteristic* pCharacteristic;

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
        if ((millis() - lastTime) > timerDelay) {
            std::string value = pCharacteristic->getValue();
            onValueReceived(value);
            lastTime = millis();
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
    pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_CHANNEL_A, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    pCharacteristic->setValue("0");
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

void onValueReceived(std::string value) {
    String readValue = value.c_str();
    Serial.print("readValue ==> ");
    Serial.println(readValue);

    byte ledvibOutput = LOW;
    if (readValue == "1") {
        ledvibOutput = HIGH;
    }
    digitalWrite(PIN_GPIO_CHANNEL_A, ledvibOutput);

    pCharacteristic->notify();
}
