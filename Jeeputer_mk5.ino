//Jeeputer Teensy4.0 Firmware Mk5
//Author: Isaac Fortner, 2021.
//Design Intent: I'm using the Teensy more as a router relaying signals to and from the Nextion display rather than using the Teensy as the real brains of the computer.
//All graphics are stored and rendered on the Nextion display, and the clock uses the built-in Nextion RTC. The Nextion Arduino library sucks, and it really doesn't add
//very much anyway, so I'm not using it. The only libraries used are for interfacing with the atmospheric sensors.
//The code is intentionally very verbose to make it easier for modification and comments. Also, I'm not a software engineer. :D 
//Please feel free to condense the code down to your liking, but know that even as is, this only uses 1% of the Teensy4.0's program storage and very little RAM.


//Libraries
#include <Wire.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL3115A2.h>

//Definitions and Variables
#define dmmr 2                //Interior dimmer input signal pin (high logic)
#define awd 3                 //4WD indicator input signal pin (high logic)
#define ffog 4                //Front foglight input signal pin (high logic)
#define defst 5               //Rear defrost input signal pin (high logic)

#define rfog 12               //Rear foglight output signal pin to relay board (low logic)
#define srch 11               //Searchlight output signal pin to relay board (low logic)
#define aux1 10               //Auxillary output signal pin for future expansion (low logic)
#define aux2 9                //Auxillary output signal pin for future expansion (low logic)

#define sound0 23             //Raptor sound board trigger
#define sound1 22             //T-Rex 1 sound board trigger
#define sound2 21             //T-Rex 2 sound board trigger
#define sound3 20             //Theme sound board trigger
#define sound4 19             //Auxillary sound board trigger
#define sound5 18             //Auxillary sound board trigger
#define sound6 17             //Auxillary sound board trigger
#define sound7 16             //Auxillary sound board trigger

int frontfogstate = 0;        //Front foglight initial state OFF
int defroststate = 0;         //Rear defrost initial state OFF
int dimmerstate = 0;          //Light dimmer initial state OFF
int DmVl = 30;                //Nextion display dimmed value, used for when the headlights are on. 30% works well, but you can set it to whatever % you want.
int awdstate = 0;             //4X4 indicator initial state OFF
int tempavg = 0;              //Average inside temp taken from Teeny SoC and altimeter
int tempin = 321;             //Dummy input temp for debugging Nextion serial comms
int tempout = 123;            //Dummy outside temp for debugging Nextion serial comms
int altft = 555;              //Dummy altitude for debugging Nextion serial comms
unsigned long PrMl1 = 0;      //time variable
unsigned long PrMl2 = 0;      //time variable
const long SnsInt = 10000;    //Temperature and altitude polling interval (default is 10 seconds)
const long IndInt = 3000;     //Vehicle indicator polling for front foglights, 4x4, defrost, and dimmer (default is 3 seconds)

//Altimeter and External Temperature Probe Setup
Adafruit_MPL3115A2 baro;
#define ONE_WIRE_BUS 6
extern float tempmonGetTemp(void);
float StdP2 = 0;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup()
{
  //Pin Modes
  pinMode(ffog, INPUT_PULLUP);
  pinMode(defst, INPUT_PULLUP);
  pinMode(dmmr, INPUT_PULLUP);
  pinMode(awd, INPUT_PULLUP);
  pinMode(rfog, OUTPUT);
  pinMode(srch, OUTPUT);
  pinMode(aux1, OUTPUT);
  pinMode(aux2, OUTPUT);
  pinMode(sound0, OUTPUT);
  pinMode(sound1, OUTPUT);
  pinMode(sound2, OUTPUT);
  pinMode(sound3, OUTPUT);
  pinMode(sound4, OUTPUT);
  pinMode(sound5, OUTPUT);
  pinMode(sound6, OUTPUT);
  pinMode(sound7, OUTPUT);
  digitalWrite(rfog, HIGH);
  digitalWrite(srch, HIGH);
  digitalWrite(aux1, HIGH);
  digitalWrite(aux2, HIGH);
  digitalWrite(sound0, HIGH);
  digitalWrite(sound1, HIGH);
  digitalWrite(sound2, HIGH);
  digitalWrite(sound3, HIGH);
  digitalWrite(sound4, HIGH);
  digitalWrite(sound5, HIGH);
  digitalWrite(sound6, HIGH);
  digitalWrite(sound7, HIGH);
  pinMode(sound0, OUTPUT);
  pinMode(sound1, OUTPUT);
  pinMode(sound2, OUTPUT);
  pinMode(sound3, OUTPUT);
  pinMode(sound4, OUTPUT);
  pinMode(sound5, OUTPUT);
  pinMode(sound6, OUTPUT);
  pinMode(sound7, OUTPUT);
  digitalWrite(sound0, HIGH);
  digitalWrite(sound1, HIGH);
  digitalWrite(sound2, HIGH);
  digitalWrite(sound3, HIGH);
  digitalWrite(sound4, HIGH);
  digitalWrite(sound5, HIGH);
  digitalWrite(sound6, HIGH);
  digitalWrite(sound7, HIGH);

  //Serial communication startup
  Serial.begin(9600);                                  //PC serial monitoring over USB for debugging
  sensors.begin();                                     //Initialize OneWire sensors
  Wire.begin();                                        //Initialize the three I2C buses on the Teensy4.0
  Serial1.begin(9600);                                 //Serial communication with Nextion display
  if (! baro.begin(&Wire2)) {                          //Initialize altimeter on I2C Bus 2 and check for comms
    Serial.println("Altimer not found on I2C Bus 2 (Pins 24/25 on Teensy4.0).");
    while (1);
  }
  Serial.println("All serial and I2C communications ready!");
  float StdP = 30.16;                                  //Set the average barometric pressure in inHg for your area.
  float StdP2 = StdP * 33.864;                         //Convert inHg to hPa for altimeter calibration
  baro.setSeaPressure(StdP2);                          //Send pressure to altimeter chip
  Serial.println("Baseline altimeter atmospheric pressure set to " + String(StdP, 2) + " inHg.");
}

void loop()
{
  //Everything in here runs on a 10 second frequency to debounce the altimeter and temperature readings.
  unsigned long CrMl1 = millis();
  if (CrMl1 - PrMl1 >= SnsInt) {
    PrMl1 = CrMl1;
    altft = baro.getAltitude() * 3.281;                             //Retreive the current altitude in meters from the altimeter and convert it to ft
    sensors.requestTemperatures();
    tempavg = (tempmonGetTemp() + baro.getTemperature()) / 2;      //Average inside temperature taken from Teensy SoC and altimeter in Celcius
    tempin = (9 / 5) * tempavg + 32;                               //Inside temperature reading as taken from the Teensy CPU temperature probe
    tempout = sensors.toFahrenheit(sensors.getTempCByIndex(0));    //Outside temperature reading as taken from the DS18B20 OneWire probe
    Serial1.print("alt.val=" + String(altft));                     //Send the altimiter reading to the Nextion display.
    Serial1.write("\xFF\xFF\xFF");
    Serial1.print("tin.val=" + String(tempin));                    //Send the inside temperature reading to the Nextion display.
    Serial1.write("\xFF\xFF\xFF");
    Serial1.print("tout.val=" + String(tempout));                  //Send the outside temperature reading to the Nextion display.
    Serial1.write("\xFF\xFF\xFF");

    //PC serial monitor readings for debugging
    Serial.print("Current altitude: " + String(altft) + "Ft");
    Serial.println();
    Serial.print("Current outside temp: " + String(tempout) + "\xC2\xB0" + "F");
    Serial.println();
    Serial.print("Current avg inside temp: " + String(tempin) + "\xC2\xB0" + "F");
    Serial.println();
    Serial.println();
    Serial.println();
  }

  //Everything in here runs on a 3 second frequency as things like the foglight indicators don't need constant refreshing.
  unsigned long CrMl2 = millis();
  if (CrMl2 - PrMl2 >= IndInt) {
    PrMl2 = CrMl2;                                                 //Reset the Indicator polling timer
    frontfogstate = digitalRead(ffog);                             //Read the current state of the foglight signal (are the front foglights powered on or off?)
    defroststate = digitalRead(defst);                             //Read the current state of the defrost signal (is the rear glass defroster on or off?)
    dimmerstate = digitalRead(dmmr);                               //Read the current state of the dimmer signal (do NOT connect to the headlight switch unless you've done the relay mod!)
    awdstate = digitalRead(awd);                                   //Read the current state of the 4x4 signal (I just spliced into the gauge cluster 4x4 bulb power since it's right behind the LCD screen)
    //Front foglight, defrost, 4x4, and dimmer indicators
    if (frontfogstate == LOW) {                                    //If the front foglights are on, tell the Nextion to turn on the front foglight icon.
      Serial1.print("vis frntfog,1");
      Serial1.write("\xFF\xFF\xFF");
      Serial.println("Front foglights ON");
    }
    if (frontfogstate == HIGH) {                                   //If the front foglights are off, tell the Nextion to turn off the front foglight icon.
      Serial1.print("vis frntfog,0");
      Serial1.write("\xFF\xFF\xFF");
    }
    if (defroststate == LOW) {                                     //If the defroster is on, tell the Nextion to turn on the defrost icon.
      Serial1.print("vis def1,1");
      Serial1.write("\xFF\xFF\xFF");
      Serial.println("Defroster ON");
    }
    if (defroststate == HIGH) {                                   //If the defroster is off, tell the Nextion to turn off the defrost icon.
      Serial1.print("vis def1,0");
      Serial1.write("\xFF\xFF\xFF");
    }
    if (awdstate == LOW) {                                        //If the 4x4 system is on, tell the Nextion to turn on the 4x4 icon.
      Serial1.print("vis AWD,1");
      Serial1.write("\xFF\xFF\xFF");
      Serial.println("4x4 ON");
    }
    if (awdstate == HIGH) {                                       //If the 4x4 system is off, tell the Nextion to turn off the 4x4 icon.
      Serial1.print("vis AWD,0");
      Serial1.write("\xFF\xFF\xFF");
    }
    if (dimmerstate == LOW) {                                     //If the headlights are on, tell the Nextion to dim the display.
      Serial1.print("dims=" + String(DmVl));                      //This will dim the display to whatever % you set up at the top.
      Serial1.write("\xFF\xFF\xFF");
      Serial.println("Dimmer ON");
    }
    if (dimmerstate == HIGH) {                                    //If the headlights are off, tell the Nextion to set the display to full brightness.
      Serial1.print("dims=100");
      Serial1.write("\xFF\xFF\xFF");
    }
    Serial.println();
    Serial.println();
    Serial.println();
  }

  //Everything in here runs constantly, as we want fast response to button commands on the screen.
  String Data_From_Display = "";
  if (Serial1.available())
  {
    String Data_From_Display = "";
    delay(100);
    while (Serial1.available())
    {
      Data_From_Display += char(Serial1.read());
    }


    //Rear foglight toggle
    if (Data_From_Display == "rfogOn")
    {
      digitalWrite(rfog, LOW);
      Serial.println("Rear foglights ON");
    }
    else if (Data_From_Display == "rfogOf")
    {
      digitalWrite(rfog, HIGH);
      Serial.println("Rear foglights OFF");
    }

    //Searchlight toggle
    else if (Data_From_Display == "SrChOn")
    {
      digitalWrite(srch, LOW);
      Serial.println("Searchlights ON");
    }
    else if (Data_From_Display == "SrChOf")
    {
      digitalWrite(srch, HIGH);
      Serial.println("Searchlights OFF");
    }

    //Sound FX
    else if (Data_From_Display == "RpTr")
    {
      Serial.println("Raptor sound effect");
      digitalWrite(sound0, LOW);
      delay(500);
      digitalWrite(sound0, HIGH);
    }
    else if (Data_From_Display == "Trx1")
    {
      Serial.println("TRex1 sound effect");
      digitalWrite(sound1, LOW);
      delay(500);
      digitalWrite(sound1, HIGH);
    }
    else if (Data_From_Display == "Trx2")
    {
      Serial.println("TRex2 sound effect");
      digitalWrite(sound2, LOW);
      delay(500);
      digitalWrite(sound2, HIGH);
    }
    else if (Data_From_Display == "ThMe")
    {
      Serial.println("Theme sound effect");
      digitalWrite(sound3, LOW);
      delay(500);
      digitalWrite(sound3, HIGH);
    }

    else
    {
      Serial1.write("\xFF\xFF\xFF");
    }
  }
}
