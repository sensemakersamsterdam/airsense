/**
 * Reads Nova SDS011 sensor values, and sends the results over TheThings
 * Uses the "Nova Fitness SDS dust sensors arduino library"
 * 
 * Sensemakers 2020
 */
#include "SdsDustSensor.h"
#define SDS_PIN_RX 2
#define SDS_PIN_TX 3
SdsDustSensor sds(SDS_PIN_RX, SDS_PIN_TX);

#include <TheThingsNetwork.h>
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

/**********************************
 * Packing the variables into bytes
 */
typedef union {
  float f[6];                // Assigning packed.f will also populate packed.bytes;
  unsigned char bytes[24];   // Both packed.f and packed.bytes share the same 4 bytes of memory.
} packed;
packed message;

/**
 * Setp up the sensor, switches on once a minute
 */
void setup() {
  Serial.begin(9600);
  loraSerial.begin(57600);
  ttn.showStatus();
  ttn.join(appEui, appKey);
  sds.begin();
  sds.setCustomWorkingPeriod(5);
}

/**
 * Checks every second
 * Most of the time the sensor will be off though 
 * because the sensor only switches on once n minutes
 * A message is sent to TheThings when we have a valid
 * measurement set.
 */
void loop() {
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
    // Get location
    // Get humidity & temperature
    // payload for TheThings
    byte message[16];
    message.f[0] = latitude;
    message.f[1] = longitude;
    ttn.sendBytes(payload, sizeof(payload));
  }
  delay(1000);
}
