// Reciever Heltec Wifi32 V3 

#define HELTEC_POWER_BUTTON

#include "heltec_unofficial.h"
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <driver/adc.h>
#include <chrono>
#include <iostream>
#include "humidity_hound_bitmap.h"

void showTemperatureAndHumidity();

bool newSignalReceived = false;
int count = 0;
float temperatureC = 0.0;
float temperatureF = 0.0;
float humidity = 0.0;
String transmitterBatt = "";
int Rssi = 0;
String signalQuality = "";
std::chrono::time_point<std::chrono::steady_clock> lastChargeUpdate = std::chrono::steady_clock::now();
const std::chrono::milliseconds CHARGE_UPDATE_INTERVAL_MS(120000); 
std::chrono::time_point<std::chrono::steady_clock> timeReceived;
int batteryPercentage = 100;
int previousBatteryPercentage = 100;
bool bleConnected = false;
bool dataReceived = false;
bool displayIsF = true;
bool isDisplayOn = true;
int sysIdentifier = 1234;
float filteredBatteryVoltage = heltec_vbat();
const float alpha = 0.9;


BLEServer *bleServer;
BLECharacteristic *bleCharacteristic;
#define SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef12-3456-789a-bcde-1234567890ab"



void disableChargingLED() {
    pinMode(13, OUTPUT);  
    digitalWrite(13, LOW);  


class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleConnected = true;
    showTemperatureAndHumidity();
  }
  void onDisconnect(BLEServer* pServer) {
    bleConnected = false;
    showTemperatureAndHumidity();
  }
};




void setupBLE() {
  BLEDevice::init("Heat Hound");
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *service = bleServer->createService(SERVICE_UUID);
  bleCharacteristic = service->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                      );
  service->start();
  BLEAdvertising *bleAdvertising = BLEDevice::getAdvertising();
  bleAdvertising->start();
}




void updateBLEData() {
    String data = "Temp: " + String(temperatureC, 1) + "C, Humidity: " + String(humidity, 1) + 
                "%, RX Batt: " + String(previousBatteryPercentage) + "%, TX Batt: " + transmitterBatt + "%" + ", Signal Quality: " + signalQuality; 
  bleCharacteristic->setValue(data.c_str());
  bleCharacteristic->notify();
}


void determineSignalQuality(){
  int rssiValue = Rssi;
  auto currentTime = std::chrono::steady_clock::now();
  auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(currentTime-timeReceived);

  
  if (timeDiff.count() < 20){
      if (rssiValue >= -70) {
        signalQuality = "Excellent";
        
      }else if (rssiValue < -70 && rssiValue >= -90){
        signalQuality = "Good";
        
      } else if (rssiValue < -90 && rssiValue >= -110){
        signalQuality = "Moderate";
        
      } else if (rssiValue < -110 && rssiValue >= -125){
        signalQuality = "Weak";
        
      } else{
        signalQuality = "Poor";
      }
  }else{
    signalQuality = "No Signal";
  } 
}



void loraRadioSetup() {
  Serial.println("Radio Init");
  
  // Set Lora Parameters
  if (radio.begin() == RADIOLIB_ERR_NONE) {
    radio.setFrequency(905.0); 
    radio.setBandwidth(67.5);
    radio.setSpreadingFactor(12);
    radio.setPreambleLength(8);
    radio.setSyncWord(0x12);
    radio.setCodingRate(5);  
    radio.startReceive();
  } else {
    Serial.println("Lora Init Failed");
  }
}


void testingDistance() {
    int rssiValue = radio.getRSSI();
    int snr = radio.getSNR();
      String outputdata = "RSSI: " + String(rssiValue) +
                      ", Temp: " + String(temperatureC) + "C" +
                      ", Humidity: " + String(humidity) + "%" +
                      ", TX Batt: " + transmitterBatt + "%" +
                      ", Count: " + String(count) +
                      ", SNR: " + String(snr) +
                      ", Signal Quality:" + signalQuality;
    Serial.println(outputdata);
    newSignalReceived = false;
}


// Boot up Splash Screen
void showSplashScreen() {
  display.clear();
  display.drawXbm(0, 0, humidityHoundGraphic_width, humidityHoundGraphic_height, humidityHoundGraphic_bits);
  display.display(); 
}




void receiveLoRaData() {
  String receivedData;
  int16_t state = radio.receive(receivedData);

  radio.startReceive();
  
  if (state == RADIOLIB_ERR_NONE) {
    radio.startReceive();

    int tempIndex = receivedData.indexOf("T:");
    int humIndex = receivedData.indexOf(",H:");
    int countIndex = receivedData.indexOf(",C:");
    int batIndex = receivedData.indexOf(",B:");
    
    if (tempIndex != -1 && humIndex != -1 && countIndex != -1) {

      int Rssi = radio.getRSSI();
      temperatureC = receivedData.substring(tempIndex + 2, humIndex).toFloat();
      
      humidity = receivedData.substring(humIndex + 3, countIndex).toFloat();
      
      count = receivedData.substring(countIndex + 3).toInt();

      transmitterBatt = receivedData.substring(batIndex + 3);
      
      temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;

      if (!isnan(temperatureC) && !isnan(humidity)) {
      timeReceived = std::chrono::steady_clock::now();
      dataReceived = true;
      showTemperatureAndHumidity();
      updateBLEData();
      newSignalReceived = true;
      } 
    }
  }
}



int calculateBatteryPercentage(float voltage) {
  return heltec_battery_percent(voltage);
}


float readBatteryVoltage() {
  float currentVoltage = heltec_vbat();
  filteredBatteryVoltage = alpha * filteredBatteryVoltage + (1 - alpha) * currentVoltage;
  return filteredBatteryVoltage;
}




bool isTimeToUpdateCharge() {
    auto now = std::chrono::steady_clock::now();
    auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(now - lastChargeUpdate);
    if (timeDiff.count() >= 30) { // Update every 30 seconds
        lastChargeUpdate = now;
        return true;
    }
    return false;
}


bool isCharging(float voltage) {
    return voltage > 4.1; 
}



void drawBatteryRx() {
    float batteryVoltage = readBatteryVoltage();
    bool charging = isCharging(batteryVoltage);
    int newBatteryPercentage = calculateBatteryPercentage(batteryVoltage);

    if (charging) {
        disableChargingLED();

        auto currentTime = std::chrono::steady_clock::now();
        if (newBatteryPercentage > previousBatteryPercentage &&
            currentTime - lastChargeUpdate >= CHARGE_UPDATE_INTERVAL_MS) {
            previousBatteryPercentage++; 
            lastChargeUpdate = currentTime;  
        }
    } else {
        pinMode(13, INPUT);  
        
        if (newBatteryPercentage != previousBatteryPercentage) {
            previousBatteryPercentage = newBatteryPercentage;
        }
    }

    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(100, 0, String(previousBatteryPercentage) + "%");

    int x = 105;
    int y = 3;
    display.drawRect(x, y, 20, 8);
    display.fillRect(x + 20, y + 2, 2, 4);

    int batteryWidth = map(previousBatteryPercentage, 0, 100, 0, 18);
    display.fillRect(x + 1, y + 1, batteryWidth, 6);
}



void drawBatteryTx() {

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(65, 0, transmitterBatt + "%");

  int txBatt = transmitterBatt.toInt();
  
  int x = 17;
  int y = 3;
  
  display.drawRect(x, y, 20, 8);
  display.fillRect(x + 20, y + 2, 2, 4);
  
  int batteryWidth = map(txBatt, 0, 100, 0, 18);
  display.fillRect(x + 1, y + 1, batteryWidth, 6); 


void showTemperatureAndHumidity() {
  display.clear();

  drawBatteryRx();
  drawBatteryTx();
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 13,"Sig: " + signalQuality);
  display.drawString(14, 31, "TEMP");

  display.setFont(ArialMT_Plain_24);
  
  if (displayIsF){
  display.drawString(0, 40, String(temperatureF, 1));  
  display.setFont(ArialMT_Plain_10);  
  display.drawString(47, 40, "F"); 
  } else {
  display.drawString(0, 40, String(temperatureC, 1));  
  display.setFont(ArialMT_Plain_10);  
  display.drawString(47, 40, "C"); 
  }


  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(113, 31, "HUM %");

  display.setFont(ArialMT_Plain_24);
  
  display.drawString(110, 40, String(humidity, 1));
  display.setFont(ArialMT_Plain_16); 
  display.drawString(128, 43, "%");

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "TX:");
  

  display.setTextAlignment(TEXT_ALIGN_LEFT);

  if (bleConnected) {
    // Show Bluetooth connected symbol
    display.drawXbm(112, 13, 16, 16, bluetoothConnectedBitmap);
  } else {
    // Show Bluetooth disconnected symbol
    display.drawXbm(112, 13, 16, 16, bluetoothDisconnectedBitmap);
  }

  display.display();
}






void setup() {

  disableChargingLED();
  heltec_setup();
  loraRadioSetup();
  setupBLE();
  
  while(!dataReceived){
  showSplashScreen();
  receiveLoRaData();
  }

  showTemperatureAndHumidity();
}



void loop() {
  
  heltec_loop();
  receiveLoRaData();
  determineSignalQuality();
  updateBLEData();
  showTemperatureAndHumidity();
  testingDistance();
  
}
