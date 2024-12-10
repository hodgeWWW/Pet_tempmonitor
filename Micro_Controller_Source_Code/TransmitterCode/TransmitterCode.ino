// Transmitter Heltec Wireless Stick V3

#define HELTEC_WIRELESS_STICK
#define HELTEC_POWER_BUTTON

#include <heltec_unofficial.h>
#include <RadioLib.h>
#include <DHTesp.h>

#define FREQUENCY           905.0
#define BANDWIDTH           67.5
#define SPREADING_FACTOR    12
#define CODING_RATE         5
#define PREAMBLE_LENGTH     8
#define SYNC_WORD           0x12
#define TRANSMIT_POWER      20

#define EXTERNAL_LED_PIN    47   
#define POWER_BUTTON_PIN    0    

DHTesp dht;
#define DHT_PIN 6

int batteryPercentage = 100;
int previousBatteryPercentage = 100;
float filteredBatteryVoltage = heltec_vbat();
int sysIdentifier = 1234;
int messageCount = 0;
float TEMP = 0;
float HUM = 0;
const float alpha = 0.9;

unsigned long buttonPressStart = 0; 
bool ledState = true; 

void setup() {
  heltec_setup();

  dht.setup(DHT_PIN, DHTesp::DHT22);

  pinMode(EXTERNAL_LED_PIN, OUTPUT);
  digitalWrite(EXTERNAL_LED_PIN, HIGH); 

  pinMode(POWER_BUTTON_PIN, INPUT_PULLUP);

  loraSetup();

  Serial.begin(115200); 

int calculateBatteryPercentage(float voltage) {
  return heltec_battery_percent(voltage);
}

float readBatteryVoltage() {
  float currentVoltage = heltec_vbat();
  filteredBatteryVoltage = alpha * filteredBatteryVoltage + (1 - alpha) * currentVoltage;
  return filteredBatteryVoltage;
}

bool isCharging(float voltage) {
  return voltage > 4.2; // Assume charging if voltage exceeds 4.2V
}

void updateBatteryPercentage() {
  float batteryVoltage = readBatteryVoltage();
  bool charging = isCharging(batteryVoltage);
  int newBatteryPercentage = calculateBatteryPercentage(batteryVoltage);

  if (charging) {
    unsigned long currentTime = millis();
    if (newBatteryPercentage > previousBatteryPercentage &&
        currentTime - buttonPressStart >= 120000) {
      previousBatteryPercentage++;  
      buttonPressStart = currentTime; 
    }
  } else {
    if (newBatteryPercentage != previousBatteryPercentage) {
      previousBatteryPercentage = newBatteryPercentage;
    }
  }
}

void getHumAndTemp() {
  TEMP = dht.getTemperature();
  HUM = dht.getHumidity();

  if (isnan(TEMP) || isnan(HUM)) {
    Serial.println("Failed to read from DHT sensor!");
    heltec_delay(2000);
    return; 
  }
}

void loraSetup() {
  Serial.println("Initializing LoRa");
  if (radio.begin() == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init succeeded");

    radio.setFrequency(FREQUENCY);
    radio.setBandwidth(BANDWIDTH);
    radio.setSpreadingFactor(SPREADING_FACTOR);
    radio.setCodingRate(CODING_RATE);
    radio.setPreambleLength(PREAMBLE_LENGTH);
    radio.setSyncWord(SYNC_WORD);
    radio.setOutputPower(TRANSMIT_POWER);
  } else {
    Serial.println("LoRa init failed");
    while (true);
  }
}

void loraTransmit() {
  updateBatteryPercentage(); // Ensure battery percentage is updated

  String message = "T:" + String(TEMP, 1) + ",H:" + String(HUM, 1) + ",C:" + String(messageCount) + ",B:" + String(previousBatteryPercentage);

  Serial.printf("Sending message: %s\n", message.c_str());
  int state = radio.transmit(message);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Transmission successful!");
    messageCount++;
  } else {
    Serial.printf("Transmission failed, code: %d\n", state);
  }

  heltec_delay(4000);
}

void checkPowerButton() {
  bool buttonPressed = (digitalRead(POWER_BUTTON_PIN) == LOW);

  if (buttonPressed) { 
    if (buttonPressStart == 0) {
      buttonPressStart = millis(); 
    }

    if (millis() - buttonPressStart >= 4000) {
      if (ledState) {
        digitalWrite(EXTERNAL_LED_PIN, LOW); 
        ledState = false; 
        Serial.println("External LED turned off after 4-second hold.");
      }
    }
  } else {
    buttonPressStart = 0;

    if (!ledState) {
      digitalWrite(EXTERNAL_LED_PIN, HIGH); // Turn on LED if it was off
      ledState = true;
      Serial.println("External LED turned back on after button release.");
    }
  }
}

void loop() {
  heltec_loop();

  checkPowerButton();

  getHumAndTemp();

  loraTransmit();
}
