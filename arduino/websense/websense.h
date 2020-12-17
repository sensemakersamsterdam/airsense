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

/**
 * Sensorthings stuff
 */
struct Thing {
  String name;
  String description;
};

struct UnitofMeasurement {
  String name;
  String symbol;
  String uri;
};

struct Datastream{
  String name;
  String description;
  String observationtype;
  UnitofMeasurement unit;
};

struct Observation {
  float result;
};
