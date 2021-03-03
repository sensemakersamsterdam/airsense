/**************************************************************************
 * Airsense is an application to measure air quality and to send the data
 * over TheThings. Intended for mobile use it supports gps for location. If
 * it is not used mobile then a gps is still usefull for timing since it
 * cannot get correct time from a time server on the internet.
 */
#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_BMP085.h>
#include <TTN_esp32.h>
#include <TinyGPS++.h>
#include <U8x8lib.h>
#include <DHT.h>
#include <SdsDustSensor.h>
#include "airsense.h"

int8_t   retries    = 10;
uint32_t retryDelay = 10000;

TTN_esp32 ttn;
Adafruit_BMP085 bmp;
HardwareSerial GPSSerial(1);
HardwareSerial SDSserial(2);
SdsDustSensor sds(SDSserial);
// DHT dht(DHT_PIN, DHT_TYPE, 28); // the last parameter is some weird delay number (24)
DHT dht(DHT_PIN, DHT_TYPE, 24); // the last parameter is some weird delay number (24)

const int sampleinterval = 5; // sample every 5 minutes
const boolean useOTAA = false;
const unsigned long gpswait = 1000;
unsigned long lastsent;
const float mindist = 250.0;

/***********
 * GPS stuff
 */
TinyGPSPlus gps;
TinyGPSCustom viewGPS(gps, "GPGSV", 3);

/**
 * Screen and LEDs
 */
#define BUILTIN_LED 25
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

/**********************************
 * When receciving a message
 */
void message(const uint8_t* payload, size_t size, int rssi)
{
  Serial.println("-- MESSAGE");
  Serial.print("Received " + String(size) + " bytes RSSI=" + String(rssi) + "db");
  for (int i = 0; i < size; i++) {
    Serial.print(" " + String(payload[i]));
    // Serial.write(payload[i]);
  }
  Serial.println();
}

/*****************************************
 * Get the coordinates from the gps stream
 **/

boolean get_coords (unsigned long ms) {
  // Feed loop
  unsigned long start = millis();
  while (GPSSerial.available() && (millis()-start)<ms) {gps.encode(GPSSerial.read());}
  
  uint8_t year  = gps.date.year();
  uint8_t month = gps.date.month();
  uint8_t day   = gps.date.day();
  uint8_t hour  = gps.time.hour();
  uint8_t min   = gps.time.minute();
  uint8_t sec   = gps.time.second();
 
  float latitude  = gps.location.lat();
  float longitude = gps.location.lng();
  float altitude  = gps.altitude.meters();
  float speed     = gps.speed.mps();
  float course    = gps.course.deg();
  float hdop      = gps.hdop.value()/100.0;

  int ngps = atoi(viewGPS.value());
    
  // Serial.print("GPS ");
  // Serial.print(ngps);
    
  if (latitude && longitude) {
    packed_message.f[0] = latitude;
    packed_message.f[1] = longitude;
    packed_message.f[2] = altitude;
    packed_message.f[3] = hdop;
 
    Serial.print("Location (");
    Serial.print(latitude);
    Serial.print(", ");
    Serial.print(longitude);
    Serial.print(") HDOP ");
    Serial.println(hdop);
  
    floatDisplay(1,"Lat",latitude);
    floatDisplay(2,"Lon",longitude);
    floatDisplay(3,"DOP",hdop);
    timeDisplay(7,hour, min, sec);
       
    return true;  
  } else {
    intDisplay(1,0,"GPS",ngps);
    timeDisplay(7,hour, min, sec);
    // Serial.println("No new location");
    return false;
  }
}

/******************************************************
 * Show a float value (preceded by text) on the display
 */
void floatDisplay (uint8_t l, uint8_t c, char *t, float f) {
  char s[16]; // used to sprintf for OLED display
  u8x8.setCursor(c, l);
  sprintf(s,"%s %.2f", t, f);
  u8x8.print(s);
}
void floatDisplay(uint8_t l, char *t, float f) {
  floatDisplay(l, t, f);
}

/*********************************************************
 * Show an integer value (preceded by text) on the display
 */
void intDisplay (uint8_t l, uint8_t c, char *t, uint8_t i) {
  char s[16]; // used to sprintf for OLED display
  u8x8.setCursor(c, l);
  sprintf(s,"%s %02d", t, i);
  u8x8.print(s);
}

/******************************
 * Show the time on the display
 */
void timeDisplay(uint8_t l, uint8_t hr, uint8_t mn, uint8_t sc) {
  char s[16]; // used to sprintf for OLED display
  u8x8.setCursor(0, l);
  sprintf(s, "T %.2d:%.2d:%.2d", hr, mn, sc);
  u8x8.print(s);
}

/**********************************************************************
 * Setup of the board, open GPS stram, connect to sensors, init screen.
 * Try to connect to TTN through OTAA, if this fails use ABP.
 */
void setup()
{
  Serial.begin(9600);
  Serial.println("Setup...");

  // pinMode(BUILTIN_LED, OUTPUT);

  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  GPSSerial.setTimeout(2); 
  SDSserial.begin(9600, SERIAL_8N1, SDS_RX, SDS_TX);
  // SDSserial.setTimeout(2); 

  // Set Galileo support by sending out a magic UBX-CFG-GNSS string
  // GPSSerial.write();

  /**
   * Begin sensors, set sampling period for the dust sensor
   */
  // bmp.begin();
  dht.begin();
  sds.begin();
  
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setQueryReportingMode().toString()); // ensures sensor is in 'query' reporting mode
  sds.setCustomWorkingPeriod(sampleinterval);
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 7, "Setup");
    
  /** 
   * Try to connect through OTAA for a minute. 
   */
  ttn.begin();
  ttn.onMessage(message); // Declare callback function for handling downlink messages from server

  if (useOTAA) {
    Serial.println("Joining TTN ");
    u8x8.drawString(0, 7, "Joining TTN");
    ttn.join(devEui, appEui, appKey, 0, retries, retryDelay);
    int startmillis = millis();
    while (!ttn.isJoined() && (millis()-startmillis)<(retries*retryDelay)) {
      Serial.print(".");
      delay(500);
    }
  }
  if (useOTAA && ttn.isJoined()) {
    Serial.println("\njoined !");
    u8x8.drawString(0, 0, "Airsense TTNOTAA");
  } else {
    Serial.println("Using ABP");
    ttn.personalize(devAddr, nwkSKey, appSKey);
    u8x8.drawString(0, 0, "Airsense TTNABP");
  }
  lastsent = millis();
  ttn.showStatus(); 
  Serial.println("Setup done");
}

/************************************************************************
 * Main loop, try to get a new location, if so send out the packet to TTN
 */
void loop() {
  // char* senttxt;
  /*******************
   * Get sensor values
   **/
  PmResult pm = sds.readPm();
  get_coords(gpswait);
  
  // Get temperature and humidity
  float temperature = dht.readTemperature();
  if(!isnan(temperature)) {floatDisplay(6,0,"T",temperature);}
  delay(100);
  float humidity = dht.readHumidity();
  if (!isnan(humidity)) {floatDisplay(6,8,"H",humidity);}
  delay(100);
  packed_message.f[6] = temperature; 
  packed_message.f[7] = humidity;  
    
  if (pm.isOk()) {
    float pm25 = pm.pm25;
    float pm10 = pm.pm10;
    floatDisplay(4,"pm25",pm25); 
    floatDisplay(5,"pm10",pm10);
    packed_message.f[4] = pm25; 
    packed_message.f[5] = pm10;  
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.print(pm10);
    // Get air pressure
    // float pressure     = bmp.readPressure()/100.0;
    // floatDisplay(4,"P",pressure);
    // packed_message.f[8] = pressure;   
    
    // TODO: Send out the measurement values to a web service   
    if (ttn.sendBytes(packed_message.bytes, sizeof(packed_message.bytes))) {}
  } else {
    // Serial.print(".");
  }
}
