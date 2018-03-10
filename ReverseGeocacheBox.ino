#include <EEPROM.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <LiquidCrystal.h>
#include <Servo.h> 


//-------------------------------------------------------------------------------
// Spielparameter
//-------------------------------------------------------------------------------

static const int SUCCESS_RADIUS = 15;
static const int GPS_SEARCH_TIMEOUT = 180;

//Target to search for
static const double TARGET_LAT = 49.97712, TARGET_LON = 12.214194;

// Home Adress so box will guid you home
static const double HOME_LAT = 55.420448, HOME_LON = 8.387873;

// Wenn ein neuer Setup Vorgang durchgeführt werden soll dann hier die Code_checksume ändern dann wird das eeprom neu geschrieben
static const int CODE_CHECKSUME = 1;

//-------------------------------------------------------------------------------
// Ab hier bitte nur noch ändern wenn du weißt was du tust.
//-------------------------------------------------------------------------------
//Servo parameter zum offen und geschossen justieren
static const int SERVO_LOCKED_ANGLE = 18;
static const int SERVO_UNLOCKED_ANGLE = 170;

//Adressen der Werte im EEPROM
static const int EEPROM_CHECKSUME = 1;
static const int EEPROM_SOLVED = 2;
// ------------------------------------------------------------------------------
//Definig Perifer Controll objects
// ------------------------------------------------------------------------------

double current_lat, current_lon;

Servo servo;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

/* 
GPS device hooked up on pins 8(rx) and 9(tx).
*/

TinyGPS gps;
SoftwareSerial ss(9, 8);

int SERVO_PIN = 10;

// ------------------------------------------------------------------------------
// Programm Setup 
// ------------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  display("  Willkommen   ","");
  delay(1000);
  lcd.clear();

  // GPS Baud Rate is 9600
  ss.begin(9600);

  if (EEPROM.read(EEPROM_CHECKSUME) != CODE_CHECKSUME) {
      Serial.print("setup mode");
      setupNewConfig();
  }

  int solved = (EEPROM.read(EEPROM_SOLVED) == 1);
  Serial.print(String("solved Flag = ") + solved + String("\n"));
  if (solved) {
      current_lat = HOME_LAT;
      current_lon = HOME_LON;
     display("     SCHON      ", "    GELOEST     ");
     delay(1000);     
     display("    SCHLOSS     ", "    OFFEN       ");     
     unlock();
     delay(1000);
  } else {
      current_lat = TARGET_LAT;
      current_lon = TARGET_LON;    
      lock();
  }

}

// ------------------------------------------------------------------------------
// Main Programm loop
// ------------------------------------------------------------------------------
void loop() {
  Serial.print("main loop\n");
  int solved = (EEPROM.read(EEPROM_SOLVED) == 1);
  display("Suche GPS Signal","");
  
  int gpsTimeout = GPS_SEARCH_TIMEOUT;
  
  float flat, flon;
  unsigned long fix_age = TinyGPS::GPS_INVALID_AGE; 
  // so langen wir keinen gps fix haben schleife durhclaufen
  while (fix_age == TinyGPS::GPS_INVALID_AGE) {
    //GPS Position aus den geparten nmea Daten auslesen
    gps.f_get_position(&flat, &flon, &fix_age);
    Serial.print(flat); 
    gpsTimeout --;
    display("Suche Standort..", String(" ") + gpsTimeout + String(" s  "));

    smartdelay(1000);
    
    // Wenn nach 180 Sekunden kein GPS signal gefunden wurde dann abschalten
    // evtl noch info dass die Box nach draussen muss.
    if (gpsTimeout <= 0) {
      display("GPS signal","nicht gefunden");
      delay(3000);
      display("nim mich mit","raus...");
      delay(3000);
      shutdown();
    }
  }
  
  //OK we have a GPS Postion
  
  
  while (true) {
      gps.f_get_position(&flat, &flon, &fix_age);  
      unsigned long distance = (unsigned long)TinyGPS::distance_between(flat, flon, current_lat, current_lon);
      if (solved) {
        display("Nach Hause in", String("  ") + distance + String(" Meter"));        
      } else {
        display("Ziel in ", String("  ") + distance + String(" Meter"));

      }
      delay(1000);
      smartdelay(1000);
      if (distance < SUCCESS_RADIUS) {
        if (!solved) {
           display("      ZIEL      ", "     ERREICHT   ");   
           delay (2000);
           display("  SCHLOSS WIRD  ", "    GEOEFFNET   ");   
           delay(2000);
           EEPROM.write(EEPROM_SOLVED, 1);
           solved = 1;
           unlock();
           display("   HERZLICHEN   ", "  GLUECKWUNSCH  ");        
           delay(4000);
           current_lat = HOME_LAT;
           current_lon = HOME_LON;
         }
      }
  }

  
  
}

void setupNewConfig() {
  display("SETUP MODE","");
  delay(1000);
  unlock();
  display("FILL AND CLOSE","BOX in 10 s");  
  delay(2000);
  for (int i = 10; i > 0; i--){
    display("LOCKDOWN IN", i + String(" SECONDS"));
    delay(1000);    
  }
  EEPROM.write(EEPROM_CHECKSUME, CODE_CHECKSUME);
  EEPROM.write(EEPROM_SOLVED, 0);
  display("CONFIG", "SAVED");
  lock();
  delay(1000);
  display("     LOCKED     ", "");
  delay(1000);
  shutdown();
}

void shutdown(){
     display("     BITTE     ", "  AUSSCHALTEN  ");
     
     delay(5000);
     display("", "");     

     
     // Die schleife hier ist notwendig damit an dieser stelle kein weiterer code ausgeführt wird und dass 
     // sie nicht vom Compiler wegoptimiert wird
     int i = 0;
     while (true) {
       i++;
       i--;
    }   
}

// ------------------------------------------------------------------------------
// set Servo to open the latch
// ------------------------------------------------------------------------------
void unlock() {
  ss.end();
  delay(100);  
  servo.attach(SERVO_PIN);  
  delay(100);
  // zwischen 0 und 180 Grad
  servo.write(SERVO_UNLOCKED_ANGLE);
  // servo bracuht Zeit die Position anzufahren
  delay(500);
  servo.detach();
  delay(100);
  ss.begin(9600);
}

// ------------------------------------------------------------------------------
// set Servo to close the latch
// ------------------------------------------------------------------------------
void lock() {
  // Weil softwareserial und servo sich gegenseitig stören muss erst softwareserial 
  // abgeschaltet werden bevor der Servo verwendet werde darf
  ss.end();
  delay(100);
  servo.attach(SERVO_PIN);
  delay(100);  
  servo.write(SERVO_LOCKED_ANGLE);
  // servo bracuht Zeit die Position anzufahren
  delay(500);  
  servo.detach();
  delay(100);
  ss.begin(9600);
}

// ------------------------------------------------------------------------------
// Displays line 1 and 2 on the lcd screen
// ------------------------------------------------------------------------------
void display(String line1 , String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);       
  lcd.print(line1);
  lcd.setCursor(0, 1);       
  lcd.print(line2);
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
    {
      char c = ss.read();
      gps.encode(c);
      Serial.print(c);
    }
  } while (millis() - start < ms);
}


