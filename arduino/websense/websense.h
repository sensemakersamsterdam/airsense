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
