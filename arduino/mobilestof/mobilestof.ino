/**
 * Reads Nova SDS011 sensor values, and sends the results over TheThings
 * Uses the "Nova Fitness SDS dust sensors arduino library"
 * 
 * Sensemakers 2020
 */
#include <TheThingsNetwork.h>
#define loraSerial Serial1
#define debugSerial Serial
#define freqPlan TTN_FP_EU868
TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);
const char *appEui = "0000000000000000";
const char *appKey = "00000000000000000000000000000000";

#include "SdsDustSensor.h"
#define SDS_PIN_RX 2
#define SDS_PIN_TX 3
SdsDustSensor sds(SDS_PIN_RX, SDS_PIN_TX);

#include "DHT.h"
#define DHT_PIN 4
DHT dht(DHT_PIN, DHT22, 24);
/***********
 * GPS stuff
 */
#include <TinyGPS++.h>
const unsigned long gpswait = 1000;
TinyGPSPlus gps;
TinyGPSCustom viewGPS(gps, "GPGSV", 3);
#define GPS_RX 5
#define GPS_TX 6
#define GPSSerial Serial

/**********************************
 * Packing the variables into bytes
 */
typedef union {
  float f[8];                // Assigning packed.f will also populate packed.bytes;
  unsigned char bytes[32];   // Both packed.f and packed.bytes share the same 4 bytes of memory.
} packed;
packed message;

/**
 * Setp up the sensor, switches on once a minute
 */
void setup() {
  loraSerial.begin(57600);
  debugSerial.begin(9600);
  ttn.onMessage(receiveMessage);
  debugSerial.println("-- STATUS");
  ttn.showStatus();
  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);
  sds.begin();
  sds.setCustomWorkingPeriod(5);
  dht.begin();
}

/**
 * Checks every second
 * Most of the time the sensor will be off though 
 * because the sensor only switches on once n minutes
 * A message is sent to TheThings when we have a valid
 * measurement set.
 */
void loop() {
  // keep listening to gps and updating location
  boolean newLocation = get_coords(gpswait);
  
  PmResult pm = sds.readPm();
  if (pm.isOk() && newLocation) {
    float pm25 = pm.pm25;
    float pm10 = pm.pm10;
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm10);
    // Get location
    // Get humidity & temperature
    float temperature = dht.readTemperature();
    delay(100);
    float humidity = dht.readHumidity();
    message.f[4] = pm25;
    message.f[5] = pm10;
    message.f[6] = temperature;
    message.f[7] = humidity;
    ttn.sendBytes(message.bytes, sizeof(message.bytes));
  }
  // delay(1000);
}

void receiveMessage(const byte *payload, size_t length, port_t port)
{
  debugSerial.println("-- MESSAGE");
  // Only handle messages of a single byte
  if (length != 1) { return; }

  if (payload[0] == 0) {
    debugSerial.println("LED: off");
    digitalWrite(LED_BUILTIN, LOW);
  } else if (payload[0] == 1) {
    debugSerial.println("LED: on");
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

boolean get_coords (unsigned long ms) {
  // Feed loop
  unsigned long start = millis();
  while (GPSSerial.available() && (millis()-start)<ms) gps.encode(GPSSerial.read());

  uint8_t year  = gps.date.year();
  uint8_t month = gps.date.month();
  uint8_t day   = gps.date.day();
  uint8_t hour  = gps.time.hour();
  uint8_t min   = gps.time.minute();
  uint8_t sec   = gps.time.second();
 
  float latitude  = gps.location.lat();
  float longitude = gps.location.lng();
  float altitude  = gps.altitude.meters();
  float hdop      = gps.hdop.value()/100.0;

  if (latitude && longitude) {
    message.f[0] = latitude;
    message.f[1] = longitude;
    message.f[2] = altitude;
    message.f[3] = hdop;       
    return true;  
  } else {
    Serial.println("No new location");
    return false;
  }
}
