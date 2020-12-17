/****************************************
 * Wemos webserver met een stofsensor
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include "SdsDustSensor.h"
#include "websense.h"

#define SDS_PIN_RX D6
#define SDS_PIN_TX D7

/**
 * Sensor definitions for klik sensorthings
 */
const int nthings = 1;
Thing things[nthings];

const int nstreams = 5;
Datastream streams[nstreams];

/*********************************
 * Time series arrays
 */
float pm10,pm25;
int isample = 0;
const int nsamples = 50;
float pm25TS[nsamples];
float pm10TS[nsamples];
String timeTS[nsamples];

const int waitfor = 1000;
const int sampleinterval = 1; // sample every 5 minutes
String webString = "";

const char* ssid = "********";
const char* password = "********";
const char* sitename = "stof";

ESP8266WebServer server(80);
SdsDustSensor sds(SDS_PIN_RX, SDS_PIN_TX);

/*****************************************************************
 * To get time from an NTP server
 **/
unsigned int localPort = 2390;
// Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
const unsigned long seventyYears = 2208988800UL;
unsigned long epoch, epoch0; // Unix time in seconds since 1970
unsigned long timechecked;
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

/************************************
 * Setup
 **/
void setup() {
  Serial.begin(9600);
  Serial.println("Start Setup");
  sds.begin();
  sds.setCustomWorkingPeriod(sampleinterval);
  
  // Connect to WiFi network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" as ");
  Serial.println(sitename);
  WiFi.hostname(sitename);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  server.on("/", []() {
    String message = "<html><head>";
    message += styleHeader();
    message += "</head><body>";
    message += "<table>";
    message += "<p>Sensors</p>";
    message += tableHead("parameter", "value");
    message += tableRow("PM25 ", (String)pm25);
    message += tableRow("PM10 ", (String)pm10);
    message += tableRow("time",   printDateTime(epoch));
    message += "</table>";
    message += "</body></html>";
    server.send(200, "text/html", message);
    Serial.println("Request for / handled");
  });

  /** 
   * Sensorthings stuff
   */
  server.on("/sensorthings/v1.0" , []() {
    String message = "{ \"value\": [";
    message += "  {\"name\": \"Things\", \"url\": \""+server.uri()+"/sensorthings/v1.0/Things\" },";
    message += "  {\"name\": \"Datastreams\", \"url\": \""+server.uri()+"/sensorthings/v1.0/Datastreams\" },";
    message += "  {\"name\": \"Observations\", \"url\": \""+server.uri()+"/sensorthings/v1.0/Observations\" },";
    message += "  {\"name\": \"FeaturesOfInterest\", \"url\": \""+server.uri()+"/sensorthings/v1.0/FeaturesOfInterest\" }";
    message += "] }";
    server.send(200, "application/json", message );
  });
  server.on("/sensorthings/v1.0/Things", []() {
    String ID = server.arg("ID");
    /** if (ID == "on") doeiets; **/
    String message = "{ \"value\": [";
    for (int i=0;i<nthings;i++) {
       if (i>0) {message += ", ";}
       message += "{ \"name\": \""+things[i].name+"\", \"description\": \""+things[i].description+"\" }";
    }
    message += "] }";
    server.send(200, "application/json", message );
  });
  /**
   * Send out *all* observations in memory
   */
  server.on("/sensorthings/v1.0/Observations", []() {
    String message = "{ \"value\": [";
    for (int i=0;i<nsamples;i++) {
       int is = ( i + isample) % nsamples;
       if (i>0) {message += ", ";}
       message += "{ \"@iot.id\": \"pm25s"+(String)(is)+"\", \"result\": \""+(String)pm25TS[is]+
                    "\", \"phenomenonTime\": \""+timeTS[is]+
                    "\", \"unitOfMeasurement\": \"ppm\"}, ";
       message += "{ \"@iot.id\": \"pm10s"+(String)(is)+"\", \"result\": \""+(String)pm10TS[is]+
                    "\", \"phenomenonTime\": \""+timeTS[is]+
                    "\", \"unitOfMeasurement\": \"ppm\"}";
    }
    message += "] }";
    server.send(200, "application/json", message );
  });
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  udp.begin(localPort);
  // Serial.println(udp.localPort());
  boolean gottime = getTimeNTP();

  pinMode(LED_BUILTIN, OUTPUT);
}

/**************************************************
 * Main loop, only gets the time and sensor reading
 */
void loop() {
  /**
   * Get current time
   **/
  unsigned long e = getTime();
  DateTime now = epoch2datetime(e);

  /*************************
   * Handle a server request
   **/
  server.handleClient();

  /*******************
   * Get sensor values
   **/
  // getDust();
  PmResult pm = sds.readPm();
  if (pm.isOk()) {
    pm25 = pm.pm25;
    pm10 = pm.pm10;
    Serial.print("PM2.5 = ");
    Serial.print(pm25);
    Serial.print(", PM10 = ");
    Serial.print(pm10);
    // Get the current time
    epoch  = getTime();
    Serial.print(", time = ");
    Serial.println(printDateTime(epoch));
    // store the current values into the array
    pm25TS[isample] = pm25;
    pm10TS[isample] = pm10;
    timeTS[isample] = printDateTime(epoch);
    isample++;
    if (isample==nsamples) {isample=0;};
  } else {
    Serial.print(".");
    // Serial.println("Error getting dust values");
  }
}

/*****************************************************************
 * Send out the stylesheet header
 */
String styleHeader() {
  String out = "<style>";
  out += " body {background-color: #ffffff; font-family: sans-serif; font-size: 24pt;}";
  out += " table {width: 80%; margin-left:auto; margin-right:auto; font-size: 24pt;}";
  out += " th, td {border-bottom: 1px solid #ddd;}";
  out += " tr:nth-child(odd) {background-color: #f2f2f2}";
  out += " .buttonOn    {background-color: #4CAF50; border-radius: 10%; font-size: 24pt;}";
  out += " .buttonMaybe {background-color: #008CBA; border-radius: 10%; font-size: 24pt;}";
  out += " .buttonOff   {background-color: #f44336; border-radius: 10%; font-size: 24pt;}";
  out += "</style>";
  out += "<meta http-equiv=\"refresh\" content=\"60; url=/\">";
  return out;
}


/*****************************************************************
 * Print the table header
 * p  parameter
 * v  value
 */
String tableHead(char* p, char* v) {
  String out = "<tr><th>";
  out += p;
  out += "</th><th>";
  out += v;
  out += "</th></tr>";
  return out;
}

/*****************************************************************
 * Print a single table row with a parameter, value pair
 * t  text
 * v  value
 */
String tableRow(String t, String v) {
  String out = "<tr><td>";
  out += t;
  out += "</td><td>";
  out += v;
  out += "</td></tr>";
  return out;
}

/*****************************************************************
 * getTime() gets a rough time from the number of seconds running
 */
unsigned long getTime() {

  unsigned long now = millis();
  unsigned long sincecheck = now - timechecked;
  unsigned long newepoch;

  /**
   *  sincecheck should be positive or else millis() has reset to zero
   *  If we have had a millis() rollover we get a new sync from NTP and a new timechecked
   */
  if (sincecheck < 0) {
    getTimeNTP();
    sincecheck = now - timechecked;
  }
  newepoch = epoch0 + sincecheck / 1000;
  // Serial.println(printDateTime(newepoch));
  return newepoch;
}

/*****************************************************************
 * getNTPTime gets the time from an NTP server
 */
boolean getTimeNTP() {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  Serial.print("Get time from ");
  Serial.println(timeServerIP);

  sendNTPpacket(timeServerIP);
  delay(waitfor);

  int cb = udp.parsePacket();
  while (!cb) { // keep requesting if there is no answer in a second
    sendNTPpacket(timeServerIP);
    Serial.print(".");
    delay(waitfor);
    cb = udp.parsePacket();
  }
  Serial.println("");
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  Serial.print("Seconds since Jan 1 1900 = " );
  Serial.println(secsSince1900);

  // now convert NTP time into everyday time:
  // Serial.print("Unix time = ");
  // subtract seventy years:
  epoch0 = secsSince1900 - seventyYears;
  Serial.println(printTime(epoch0));
  timechecked  = millis();
  Serial.print("Time running ");
  Serial.println ((String)(timechecked / 1000.0));
  epoch = epoch0;
  return true;
}
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  // Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

DateTime epoch2datetime(uint32_t e) {
  DateTime rtc;
  // - Convert seconds to year, day of year, day of week, hours, minutes, seconds
  rtc.dsec = e % 86400;  // Seconds since midnight
  rtc.sec  = e % 60;
  rtc.min = e % 3600 / 60;
  rtc.hour = e % 86400 / 3600;
  rtc.dow = ( e % (86400 * 7) / 86400 ) + 4;  
  int doy = e % (86400 * 365) / 86400;
  unsigned yr = e / (86400 * 365) + 1970;
  unsigned ly;                                        // Leap year
  for (ly = 1972; ly < yr; ly += 4) {                 // Adjust year and day of year for leap years
    if (!(ly % 100) && (ly % 400)) continue;        // Skip years that are divisible by 100 and not by 400
    --doy;                                          //
  }                                                   //
  if (doy < 0) doy += 365, ++yr;                      // Handle day of year underflow
  rtc.year = yr;
  // - Find month and day of month from day of year
  static uint8_t const dm[2][12] = {                  // Days in each month
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}, // Not a leap year
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}  // Leap year
  };                                                  //
  int day = doy;                                      // Init day of month
  rtc.month = 0;                                      // Init month
  ly = (yr == ly) ? 1 : 0;                            // Make leap year index 0 = not a leap year, 1 = is a leap year
  while (day > dm[ly][rtc.month]) day -= dm[ly][rtc.month++]; // Calculate month and day of month
  rtc.doy = doy + 1;                                  // - Make date ones based
  rtc.day = day + 1;
  rtc.month = rtc.month + 1;

  return rtc;
}  
// From "2:00:00" to 7200   
int timetosec(String d) {
  int s1 = d.indexOf(":");
  int s2 = d.indexOf(":",s1);
  int s3 = d.length();
  int h = d.substring(0,s1).toInt();
  int m = d.substring(s1+1,s2).toInt();
  int s = d.substring(s2+1,s3).toInt();
  int t = h*3600+m*60+s;
  return t;
}
String printTime(unsigned long e) {
  DateTime t = epoch2datetime(e);
  // String out = (String)t.hour + ":" + (String)t.min + ":" + (String)t.sec;
  char buff[8];
  sprintf(buff,"%02d:%02d:%02d",t.hour,t.min,t.sec);
  String out = String(buff);
  return out;
}                                                  //
String printDate(unsigned long e) {
  DateTime t = epoch2datetime(e);
  // String out = (String)t.day + "-" + (String)t.month + "-" + (String)t.year;
  char buff[10];
  sprintf(buff,"%02d-%02d-%04d",t.day,t.month,t.year);
  String out = String(buff);
  return out;
}                                                //
String printDateTime(unsigned long e) {
  DateTime t = epoch2datetime(e);
  // String out = (String)t.day + "-" + (String)t.month + "-" + (String)t.year + " ";
  // out += (String)t.hour + ":" + (String)t.min + ":" + (String)t.sec + " UTC";
  char buff[24];
  sprintf(buff,"%04d-%02d-%02dT%02d:%02d:%02dZ",t.year,t.month,t.day,t.hour,t.min,t.sec);
  String out = String(buff);
  return out;
}
