/**
 * Some constants read from a configuration file
 */
const char* ssid = "********";
const char* password = "********";
const char* sitename = "stof";
const char *mqtt_server = "test.mosquitto.org"; 
// const char *mqtt_server = "mqtt.sensemakers.org";
const char *app_id = "airsense";
const char *device_id = "1011DL143";
const float latitude=52.3757, longitude=4.9083;

/**
 * Time and date stuff
 **/
struct DateTime {
  int sec;
  int dsec; // seconds since midnight
  int min;
  int hour;
  int dow;
  int day;
  int month;
  int year;
  int config;
  int doy;    // not BCD!
};
