
#define GPS_RX  38
#define GPS_TX  39
#define SDS_RX  17
#define SDS_TX  23
#define DHT_PIN 34
#define DHT_TYPE DHT22   // DHT 22  (AM2302), AM2321

/***************************************************************************
 *  TheThings
 ****************************************************************************/
// ABP
const char* devAddr = "CHANGE_ME"; // Change to TTN Device Address
const char* nwkSKey = "CHANGE_ME"; // Change to TTN Network Session Key
const char* appSKey = "CHANGE_ME"; // Change to TTN Application Session Key
// OTAA
const char* devEui = "CHANGE_ME"; // Change to TTN Device EUI
const char* appEui = "CHANGE_ME"; // Change to TTN Application EUI
const char* appKey = "CHANGE_ME"; // Chaneg to TTN Application Key

/**********************************
 * Packing the variables into bytes
 */
typedef union {
  float f[8];                // Assigning fVal.f will also populate fVal.bytes;
  unsigned char bytes[32];   // Both fVal.f and fVal.bytes share the same 4 bytes of memory.
} floatArr2Val;
floatArr2Val packed_message;
