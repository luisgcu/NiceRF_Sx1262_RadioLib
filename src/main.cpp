#include <Arduino.h>
#include <RadioLib.h>
// ============================================================================
// FREQUENCY SELECTION - Choose one based on your region
// ============================================================================
//#define FREQUENCY 434.5 // 434.5MHz - Europe/Asia ISM band
#define FREQUENCY 915.0   // 915MHz - Americas ISM band
#define POWER 10 // -9~22 dBm

// ============================================================================
// LORA CONFIGURATION PARAMETERS
// ============================================================================
#define TCXO_VOLTAGE 3.3    // TCXO voltage in V (1.8V or 3.3V)
#define BANDWIDTH 250.0     // Bandwidth in kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500)
#define SPREADING_FACTOR 8  // Spreading Factor (6-12)
#define CODING_RATE 5       // Coding Rate (5=4/5, 6=4/6, 7=4/7, 8=4/8)
#define TRANSMIT_INTERVAL 10000UL  // Transmission interval in milliseconds

// ============================================================================
// BOARD CONFIGURATION - Choose one ESP32 board type
// ============================================================================
//#define ESP32DEV_BOARD      // ESP32 Dev Board (GPIO16, GPIO21, GPIO17)
#define ESP32WROOM_DEV_BOARD  // ESP32-WROVER-B Dev Board (GPIO4, GPIO12, GPIO14)

#ifdef ESP32DEV_BOARD
#define NSS_PIN 5      // Chip Select (CS/SS)
#define NRESET_PIN 16  // Reset pin
#define RFBUSY_PIN 21  // Busy pin
#define DIO1_PIN 17    // DIO1 interrupt pin
#else
#define NSS_PIN 5      // Chip Select (CS/SS)
#define NRESET_PIN 4   // Reset pin
#define RFBUSY_PIN 12  // Busy pin
#define DIO1_PIN 14    // DIO1 interrupt pin
#endif

// Uncomment the following only on one of the nodes to initiate the pings
#define INITIATING_NODE

SX1262 radio = new Module(NSS_PIN, DIO1_PIN, NRESET_PIN, RFBUSY_PIN);

// Global variables
int transmissionState = RADIOLIB_ERR_NONE;
bool transmitFlag = false;
volatile bool operationDone = false;

// Interrupt handler for radio operations
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  operationDone = true;
}
void setup() {
  Serial.begin(115200);

  // Initialize SX1262
  Serial.print(F("NiceRF [SX1262] RadioLib Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  // Apply hardware settings
  radio.setTCXO(TCXO_VOLTAGE);
  radio.setDio2AsRfSwitch();
  delay(15);

  // Set LoRa configuration
  radio.setFrequency(FREQUENCY);
  radio.setOutputPower(POWER);
  radio.setBandwidth(BANDWIDTH);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  radio.setCodingRate(CODING_RATE);

  // Set interrupt handler
  radio.setDio1Action(setFlag);

#if defined(INITIATING_NODE)
  Serial.print(F("[SX1262] Sending first packet ... "));
  transmissionState = radio.startTransmit("Hello World!");
  transmitFlag = true;
#else
  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
  }
#endif
}


// Transmission timer
unsigned long lastTransmitTime = 0;
const unsigned long transmitInterval = TRANSMIT_INTERVAL;

void loop() {
  if(operationDone) {
    operationDone = false;

    if(transmitFlag) {
      // Transmission completed
      if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println(F("[SX1262] Transmission finished!"));
      } else {
        Serial.print(F("[SX1262] Transmission failed, code "));
        Serial.println(transmissionState);
      }
      radio.startReceive();
      transmitFlag = false;
    } else {
      // Reception completed
      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("[SX1262] Received packet!"));
        Serial.print(F("[SX1262] Data:\t\t")); Serial.println(str);
        Serial.print(F("[SX1262] RSSI:\t\t")); Serial.print(radio.getRSSI()); Serial.println(F(" dBm"));
        Serial.print(F("[SX1262] SNR:\t\t"));  Serial.print(radio.getSNR()); Serial.println(F(" dB"));
      }
      radio.startReceive();
    }
  }

#ifdef INITIATING_NODE
  // Send periodic packets
  if (!transmitFlag && millis() - lastTransmitTime >= transmitInterval) {
    Serial.print(F("[SX1262] Sending scheduled packet ... "));
    transmissionState = radio.startTransmit("Hello World!");
    transmitFlag = true;
    lastTransmitTime = millis();
  }
#endif
}
