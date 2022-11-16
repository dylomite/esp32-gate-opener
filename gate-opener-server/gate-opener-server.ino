#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define PIN_GPIO_CONN_LED 2
#define PIN_GPIO_CHANNEL_A 4
#define PIN_GPIO_CHANNEL_B 18
#define bleServerName "ESP32 Gate Opener BtServer"
#define SERVICE_UUID "bc5d00ca-badf-4244-ae11-4a8f73fd409f"
#define CHARACTERISTIC_UUID_CHANNEL_A "1dd23185-5d48-457c-a5e4-0f21afed2e58"
#define CHARACTERISTIC_UUID_CHANNEL_B "04889dda-9528-45fb-ac11-f522997afb0b"
#define CHARACTERISTIC_VALUE_LOW "0"
#define CHARACTERISTIC_VALUE_HIGH "1"
#define DELTA_TIME_CONN_LED_BLINK 80
#define DELTA_TIME_MS 250

unsigned long lastTimeRead = 0;  // ms
bool deviceConnected = false;
bool isAdvertising = false;

BLECharacteristic* pCharacteristicA;
BLECharacteristic* pCharacteristicB;

void setup() {
    Serial.begin(19200);
    pinMode(PIN_GPIO_CONN_LED, OUTPUT);
    pinMode(PIN_GPIO_CHANNEL_A, OUTPUT);
    pinMode(PIN_GPIO_CHANNEL_B, OUTPUT);
    setupBleServer();
    digitalWrite(PIN_GPIO_CONN_LED, LOW);
    setGpioValues(CHARACTERISTIC_VALUE_LOW, CHARACTERISTIC_VALUE_LOW);
}

void loop() {
    if (deviceConnected) {
        if ((millis() - lastTimeRead) > DELTA_TIME_MS) {
            std::string valueA = pCharacteristicA->getValue();
            std::string valueB = pCharacteristicB->getValue();
            setupOutputAndNotify(valueA, valueB);
            lastTimeRead = millis();
        }
    } else if (isAdvertising) {
        onAdvertising();
    }
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
    delay(DELTA_TIME_CONN_LED_BLINK);
    digitalWrite(PIN_GPIO_CONN_LED, LOW);
    delay(DELTA_TIME_CONN_LED_BLINK);
    digitalWrite(PIN_GPIO_CONN_LED, HIGH);
}

void setupOutputAndNotify(std::string valueA, std::string valueB) {
    setGpioValues(valueA, valueB);
    pCharacteristicA->notify();
    pCharacteristicB->notify();
}

// TODO: avoid str convert!
void setGpioValues(std::string valueA, std::string valueB) {
    String valueAStr = valueA.c_str();
    String valueBStr = valueB.c_str();

    Serial.print("valueA = ");
    Serial.print(valueAStr);
    Serial.print(" valueB = ");
    Serial.println(valueBStr);

    byte outputA = getPinOutput(valueAStr);
    byte outputB = getPinOutput(valueBStr);

    digitalWrite(PIN_GPIO_CHANNEL_A, outputA);
    digitalWrite(PIN_GPIO_CHANNEL_B, outputB);
}

byte getPinOutput(String characteristicValue) {
    byte output = LOW;
    if (characteristicValue == CHARACTERISTIC_VALUE_HIGH) {
        output = HIGH;
    }
    return output;
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
        setGpioValues(CHARACTERISTIC_VALUE_LOW, CHARACTERISTIC_VALUE_LOW);
        pServer->getAdvertising()->start();
        isAdvertising = true;
    }
};

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
