#include <SPI.h>
#include <RH_RF95.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

// Singleton instance of the radio driver
RH_RF95 rf95;
unsigned long current_millis = millis();

TinyGPS gps;
SoftwareSerial ss(4, 3);

static void smartdelay(unsigned long ms);

struct Config {
  float freq;
  float flat;
  float flong;
  uint8_t sf;
};

struct Location {
  float flat;
  float flong;
};

struct Data {
  int16_t rssi;
  int packet_id;
};

struct ID {
  int id;
};

Config con;
Location loc;
Data data;
ID packet_id;

//float flat = TinyGPS::GPS_INVALID_F_ANGLE;
//float flong = TinyGPS::GPS_INVALID_F_ANGLE;
float flat = 1.1;
float flong = 1.1;
unsigned long age = 0;

void setup() 
{
  Serial.begin(9600);
  while (!Serial);
  ss.begin(9600);
  if (!rf95.init()) while(1);
  rf95.setFrequency(863.1);
  rf95.setSpreadingFactor(12);
  rf95.setCodingRate4(4);
  rf95.setTxPower(20);   
  uint8_t mess[1] = {1}; 
  rf95.send(mess, sizeof(mess));
  rf95.waitPacketSent();
  while(flat == TinyGPS::GPS_INVALID_F_ANGLE) {smartdelay(100);gps.f_get_position(&flat, &flong, &age); loc.flat = flat; loc.flong = flong;} 
}

void loop()
{ 
  if (rf95.available()) {
  uint8_t len = sizeof(con);
  uint8_t id_len = sizeof(packet_id);
    if (rf95.recv((uint8_t*)&con, &len)) {
      con.freq =  ((float )((int)(con.freq * 10))) / 10;
      rf95.setSpreadingFactor(con.sf);
      rf95.setFrequency(con.freq);
      uint8_t mess[1] = {1}; 
      rf95.send(mess, sizeof(mess));
      rf95.waitPacketSent();
      uint8_t message[1];
//      Serial.write((uint8_t*)&loc, sizeof(loc));
//      Serial.write((uint8_t*)&con, sizeof(con));
      Serial.print("SF: "); Serial.println(con.sf);
      current_millis = millis();
      uint8_t l = sizeof(data);
      uint8_t i = 0;
      while(millis() - current_millis < 5000)  {
        if (rf95.recv((uint8_t*)&packet_id, &id_len)) {
          data = {rf95.lastRssi(), packet_id.id};
          current_millis = millis();
          i += 1;
//          Serial.write((uint8_t*)&data, l);
        }
      }
      Serial.print(i); Serial.println("/100 packets were received");
      Serial.print("Last packet_id: "); Serial.println(packet_id.id);
    }
    rf95.init();
    rf95.setFrequency(863.1);
    rf95.setSpreadingFactor(12);
    rf95.setCodingRate4(4);
    rf95.setTxPower(20);  
    delay(1);
  }
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
