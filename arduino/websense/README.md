# Websense

Code for a Wemos D1R2 ESP8266 board. This reads the sensor
measurements form a Nova SDS011 particulate matter sensor. It runs a
web server at //stof/, this endpoint shows:
* a table with latest sensor readings
* a graph of the current series of measurements
The time series is continuously updated with the latest
measurement. Time series length is configurable in the code.

![Screenshot showing table and graph](2021-01-01.png)