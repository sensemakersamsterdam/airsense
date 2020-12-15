/**
 * Reads Nova SDS011 sensor values, that's it
 * Uses the "Nova Fitness SDS dust sensors arduino library"
 * 
 * Sensemakers 2020
 */
#include "SdsDustSensor.h"
#define SDS_PIN_RX 2
#define SDS_PIN_TX 3
SdsDustSensor sds(SDS_PIN_RX, SDS_PIN_TX);

/**
 * Setp up the sensor, switches on once a minute
 */
void setup() {
  Serial.begin(9600);
  sds.begin();
  sds.setCustomWorkingPeriod(1);
}

/**
 * Checks every second
 * Most of the time the sensor will be off though 
 * because the sensor only switches on once a minute
 */
void loop() {
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    Serial.print("PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
  }

  delay(1000);
}
