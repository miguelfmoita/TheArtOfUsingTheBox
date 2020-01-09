#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <math.h>
#include "I2C_Anything.h"

#define ADDRESS_A 1
#define ADDRESS_B 2

//GPS
SoftwareSerial mySerial(3, 2);
Adafruit_GPS GPS(&mySerial);
#define GPSECHO  true
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

//constantes
const float deg2rad = 0.01745329251994;
const float rEarth = 6371000.0;  
//variaveis
float range = 0;                  
String WayPoint;
String here;
byte WP=0;

//Cenas po GPS
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
}


void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

String int2fw (int x, int n) {
  // returns a string of length n (fixed-width)
  String s = (String) x;
  while (s.length() < n) {
    s = "0" + s;
  }
  return s;
}

String gps2string (String lat, float latitude, String lon, float longitude) {
  // returns "Ndd mm.mmm, Wddd mm.mmm";
  int dd = (int) latitude/100;
  int mm = (int) latitude % 100;
  int mmm = (int) round(1000 * (latitude - floor(latitude)));
  String gps2lat = lat + int2fw(dd, 2) + " " + int2fw(mm, 2) + "." + int2fw(mmm, 3);
  dd = (int) longitude/100;
  mm = (int) longitude % 100;
  mmm = (int) round(1000 * (longitude - floor(longitude)));
  String gps2lon = lon + int2fw(dd, 3) + " " + int2fw(mm, 2) + "." + int2fw(mmm, 3);
  String myString = gps2lat + ", " + gps2lon;
  return myString;
};


float string2lat (String myString) {
  // returns radians: e.g. String myString = "N38 58.892, W076 29.177";
  float lat = ((myString.charAt(1) - '0') * 10.0) + (myString.charAt(2) - '0') * 1.0 + ((myString.charAt(4) - '0') / 6.0) + ((myString.charAt(5) - '0') / 60.0) + ((myString.charAt(7) - '0') / 600.0) + ((myString.charAt(8) - '0') / 6000.0) + ((myString.charAt(9) - '0') / 60000.0);
  //Serial.print("float lat: ");
  //Serial.println(lat);
  lat *= deg2rad;
  if (myString.charAt(0) == 'S')
    lat *= -1;                                                           // Correct for hemisphere
  return lat;
};

float string2lon (String myString) {
  // returns radians: e.g. String myString = "N38 58.892, W076 29.177";
  float lon = ((myString.charAt(13) - '0') * 100.0) + ((myString.charAt(14) - '0') * 10.0) + (myString.charAt(15) - '0') * 1.0 + ((myString.charAt(17) - '0') / 6.0) + ((myString.charAt(18) - '0') / 60.0) + ((myString.charAt(20) - '0') / 600.0) + ((myString.charAt(21) - '0') / 6000.0) + ((myString.charAt(22) - '0') / 60000.0);
  //Serial.print("float lon: ");
  //Serial.println(lon);
  lon *= deg2rad;
  if (myString.charAt(12) == 'W')
    lon *= -1;                                                           // Correct for hemisphere
  return lon;
};

//Calcula a distancia entre 2 coordenadas
float haversine (float lat1, float lon1, float lat2, float lon2) {
  // returns the great-circle distance between two points (radians) on a sphere
  float h = sq((sin((lat1 - lat2) / 2.0))) + (cos(lat1) * cos(lat2) * sq((sin((lon1 - lon2) / 2.0))));
  float d = 2.0 * rEarth * asin (sqrt(h)); 
  //Serial.println(d);
  return d;
};

void setup() {
  Serial.begin(112500);           // start serial for output
  
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);// RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);// 1 Hz update rate
  useInterrupt(true);
  
  
  Wire.begin(ADDRESS_B);
  Wire.onReceive(receiveEvent);
}





void loop() {
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))// also sets the newNMEAreceived() flag to false
      return;// We can fail to parse a sentence in which case we should just wait for another
  }
  if (GPS.fix){
    if (WP>=100){
      
      //Marco Geodesico - Puzzle
      if (WP==101){
        WayPoint = "N38 42.881, W009 09.811";
      }
      
      //Patamar - Jogo do luis
      else if (WP==102){
        WayPoint = "N38 42.882, W009 09.812";
      }
      
      //Acesso a praia - codigo na praia
      else if (WP==103){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //caminho - Cobra
      else if (WP==104){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //vale - Bueda numeros no ecra
      else if (WP==105){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Fenda - Disco
      else if (WP==106){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Ponta - Super Mario
      else if (WP==107){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Gruta - codigo na gruta
      else if (WP==108){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Pinheiro - RGB
      else if (WP==109){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Fim da subida - Jogo da avozinha
      else if (WP==110){
        WayPoint = "N38 42.883, W009 09.813";
      }
      //Rocky 1
      else if (WP==120){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Caminho Final 1 - Jogo com punicao
      else if (WP==111){
        WayPoint = "N38 42.883, W009 09.813";
      }
      //Punicao 1
      else if (WP==121){
        WayPoint = "N38 42.883, W009 09.813";
      }
      //Punicao 2
      else if (WP==131){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Caminho 2 - caixa bigodes
      else if (WP==112){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Jogo da bomba
      else if (WP==113){
        WayPoint = "N38 42.883, W009 09.813";
      }
      
      //Fechar
      else if (WP==114){
        WayPoint = "N38 42.883, W009 09.813";
      }
      here = gps2string ((String) GPS.lat, GPS.latitude, (String) GPS.lon, GPS.longitude);
      range = haversine(string2lat(here), string2lon(here), string2lat(WayPoint), string2lon(WayPoint));
      Wire.beginTransmission(ADDRESS_A);
      I2C_writeAnything (range); 
      Wire.endTransmission();  
      delay(250);
    }
    else if (WP<100 && range>-2){
      range=-2;
      Wire.beginTransmission(ADDRESS_A);
      I2C_writeAnything (range); 
      Wire.endTransmission();  
      delay(250);
    }
  } 
  else if(!GPS.fix){
    range=-1;
    Wire.beginTransmission(ADDRESS_A);
    I2C_writeAnything (range); 
    Wire.endTransmission();
  
    delay(500);
  }
}

void receiveEvent(int howMany){
  while (Wire.available() > 0){
    WP=Wire.read();
  }
}
