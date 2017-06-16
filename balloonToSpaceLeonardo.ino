#include <DFRobot_sim808.h>
#include <string.h>
#include <Servo.h>
#include <Adafruit_BMP183.h>
#include <SD.h>

//ALL GLOBALS DEFINED BEFORE SETUP

//USED I/O
//BOOT 12 
//bmp 13,10,11 8 for cs 5v gnd
//microservo 9, 5v gnd
//sd logging 14, 15, 16, 4 for cs
    //logging to SD card and bmp
    //MISO=D14
    //SCK=D15
    //MOSI=D16
    //CS=8

//serial connection for leonardo
DFRobot_SIM808 sim808(&Serial1);

//serial connection for uno
//DFRobot_SIM808 sim808(&Serial);

#define PHONE_NUMBER1 "19022373864"
//#define PHONE_NUMBER2 "19028177601"

//since the message the module will recieve are going to be small, the message length doesent have to be long
#define MESSAGE_LENGTH 16

//set the sea pressure level (changes day to day)
#define seaPressureLevel  1003

//define the pins the BMP shield will use
//5v wire is red, 3.3v wire is red with black sharpie around the top
#define BMP183_CLK  11 //blue wire
#define BMP183_SDO  10 //purple wire // AKA MISO
#define BMP183_SDI  13  //green wire // AKA MOSI
#define BMP183_CS   8  //orange wire

//intitalize the pins for the BMP sesnor with software SPI
Adafruit_BMP183 bmp = Adafruit_BMP183(BMP183_CLK, BMP183_SDO, BMP183_SDI, BMP183_CS);

//creating the microservo object for the release mechanism
Servo releaseServo;

//defining a few variables, like the timer, a counter for a second condition, and the int where the message index is stored
int messageIndex = 0;

//setting the coordinate restraints of the project, so it doesent wander too far without releasing
/*
float maxLongWest = -63.615558;
float maxLongEast = -63.612358;
float maxLatNorth = 44.670026;
float maxLatSouth = 44.667336;
*/

//setting all the buffers the program uses, in order to print/send messages (should be around 150)
//setting the main buffer
char gpsBuffer[100];
//int gpsbufcnt = 0;

//buffer used for storing converted strings
char num[16];

//buffer where recieved message is stored
//char message[MESSAGE_LENGTH];

//lat is at position 27, long is at 29
//char googleMapsURL[40] = "http://maps.google.com/?q= , "

//bool flag to know when the release mechanism is triggered
bool isReleased = false;

//boolean flag to be set true when release is reset to fully closed
bool isReleaseReset = false;


//creating the file for data to be logged to
File myFile;

  void setup() {
    //starting serial connection
    //also starts the USB connection for Leonardo
    //Serial.begin(115200);
    //delay(5000);
    //Serial.println("Arduino started.");
    
    //starts the communication between leonardo and SIM808
    //BAUD RATE MUST BE MINIMUM 9600 IN ORDER FOR GPS TO FUNCTION
    Serial1.begin(9600);

    //throwing pin 12 high is the same as pressing the boot button
    //this is useful in case the project powers off, and has to reboot again
    //only needed for uno, not leonardo, because pin becomes inactive if powered with 7-23v power supply
    //pinMode(12, OUTPUT);
    //digitalWrite(12, HIGH);
    //delay(1500);
    //digitalWrite(12, LOW);

    //starts reading the BMP sensor data
    bmp.begin();

    //initializing SD card
    SD.begin(4);

    //******** Initialize sim808 module *************
    while(!sim808.init()) {
        delay(1000);
        //Serial.print("Sim808 init error\r\n");
    }

    //Serial.println("SIM808 init completed.");

    //************* Turn on the GPS power************
    while(!sim808.attachGPS()){
        delay(1000);
        sim808.sendSMS(PHONE_NUMBER1,"Open the GPS power failure");
    }
    
    //setting the delay at which the gps recieves nmea lines to 5 seconds instead of 1 (IN dfrobot_sim808.cpp, under attachGPS implementation)
    //also excludes all line except gprmc
    
  }
  
  void loop() {

    //setting the main timers
    static unsigned long myGPSTimer = 0;  
    static unsigned long myReleaseTimer = 0;  
    unsigned long millisTimer = millis();

    //resets the release to closed position once on startup
    if (isReleaseReset == false) {
      releaseServo.attach(9);
      releaseServo.write(180);
      releaseServo.detach();
      isReleaseReset = true;
    }
    
    //calls the getGPS function constantly, even if there is no GPS fix
    sim808.getGPS();
/*   
    //*********** Detecting unread SMS ************************
    messageIndex = sim808.isSMSunread();

    //*********** At least, there is one UNREAD SMS ***********
    if (messageIndex > 0) { 
       //reusing gpsBuffer as args in this function, to save memory
       sim808.readSMS(messageIndex, message, MESSAGE_LENGTH);
                   
       //***********In order not to full SIM Memory, is better to delete it**********
       sim808.deleteSMS(messageIndex);

       //this is the check to see if the message it recieves is equal to the message you want it to receive, in order to drop the project
       if (strcmp(message, "drop") == 0 ){
         releaseServo.attach(6);
         releaseServo.write(140);
         releaseServo.detach();
         sim808.sendSMS(PHONE_NUMBER1, "package released via text message");
       }

       //send a text message rstgps in order to reset the gps module if it stops working mid flight
       if (strcmp(message, "rstgps") == 0){
          sim808.detachGPS();
          delay(3000);
          sim808.attachGPS();
       }

       //if the message is an at command, echo the response in a text message
       if (message[0] == 'A' && message[1] == 'T') {
  
        Serial1.write(message);
        Serial1.write("\r\n");
        gpsbufcnt = 0;
        int gpstimeout = millis();
        strcpy( gpsBuffer, "GPS response timeout expired." );
        while( (millis() - gpstimeout) < 1000 ) {
          while( Serial1.available() ) {
            gpsBuffer[gpsbufcnt++] = Serial1.read();
            gpsBuffer[gpsbufcnt] = '\0';
          } 
        }
        sim808.sendSMS(PHONE_NUMBER1, gpsBuffer);     
       }
     }
*/
     //for first test flight, we want to test the parachute release by grabbing the air pressure at roughly 100-500ft, and seeing if it actually triggered
     //at the right altitude
     
      //gps fence
      //checks to see if the gps fix is lost, so the balloon doesent accidentally release
      /*     
      if (sim808.GPSdata.lat != 0.000000 && sim808.GPSdata.lon != 0.000000){
        
        //conditional for fence constraint
        //if the module leaves this lattitude contraint, release from balloon
        if (sim808.GPSdata.lat >= maxLatNorth){
          //trigger release, and send message
           sim808.sendSMS(PHONE_NUMBER1, "Max north");           
        }
  
        //if the module leaves this lattitude contraint, release from balloon
        if (sim808.GPSdata.lat <= maxLatSouth){
         //trigger release, and send message
          sim808.sendSMS(PHONE_NUMBER1, "Max South");
        }
  
        //if the module leaves this longitude contraint, release from balloon
        if (sim808.GPSdata.lon <= maxLongWest){
          //trigger release, and send message
          sim808.sendSMS(PHONE_NUMBER1, "Max west"); 
        }
        
        //if the module leaves this longitude contraint, release from balloon
        if (sim808.GPSdata.lon >= maxLongEast){
          //trigger release, and send message
          sim808.sendSMS(PHONE_NUMBER1, "Max east"); 
        }
      }
      */
      

    //every x second(s), create a buffer containing all the GPS data, and print it to the serial monitor
    
    if ((millisTimer - myGPSTimer) >= 15000UL) {
      myGPSTimer = millisTimer;

      // open the file. note that only one file can be open  at a time,
      // so you have to close this onebefore opening another.
      myFile = SD.open("test.txt", FILE_WRITE);
      
      //adding GPS info to buffer
      //time
      sprintf(gpsBuffer, "%i:%i:%i \r\n", sim808.GPSdata.hour, sim808.GPSdata.minute, sim808.GPSdata.second);
      
      //lattitude
      strcat(gpsBuffer, "http://maps.google.com/?q=");
      String(sim808.GPSdata.lat,6).toCharArray(num,16);
      strcat(gpsBuffer, num);
      //writing the lat to file
      myFile.print(num);
      
      //longitude
      strcat(gpsBuffer, ",");
      String(sim808.GPSdata.lon,6).toCharArray(num,16);
      strcat(gpsBuffer, num);
      //writing the long to file
      myFile.print(",");
      myFile.print(num);

      //gps altitude
      strcat(gpsBuffer, "\r\n");
      String(sim808.GPSdata.altitude,2).toCharArray(num,16);
      strcat(gpsBuffer, num);
      //writing the altitude to file
      myFile.print(",");
      myFile.print(num);

      //adding BMP info to buffer
      //bmp altitude
      float altitude = (bmp.getAltitude(seaPressureLevel));
      strcat(gpsBuffer, "\r\n");
      sprintf(&gpsBuffer[strlen(gpsBuffer)], "%i", (int)(altitude * 100));
      myFile.print(",");
      myFile.println(altitude);

      //temperature
      float temperature = bmp.getTemperature();
      strcat(gpsBuffer, "\r\n");
      sprintf(&gpsBuffer[strlen(gpsBuffer)], "%i", (int)(temperature * 100));
      strcat(gpsBuffer, "\r\n");

      //uncomment this to have the sim808 print out the data to the serial monitor every loop
      //Serial.print(gpsBuffer);
      //Serial.print("\r\n");

      //closing the log file
      myFile.close();
      
      //checks to see if a message is able to be sent
      sim808.sendSMS(PHONE_NUMBER1,gpsBuffer);
      
      gpsBuffer[0] = '\0';
    }

    //after 10 minutes (40x15, because its dependant on the 15 second timed loop), trigger the release mechanism
    if (millisTimer - myReleaseTimer >= 600000UL && isReleased == false){
      //myReleaseTimer = millisTimer;
      isReleased = true;
      releaseServo.attach(9);
      releaseServo.write(135);
      sim808.sendSMS(PHONE_NUMBER1, "10 minutes have passed, package released");
      releaseServo.detach();
    }
    /*
    if (millisTimer - myChuteTimer >= 1200000 && sim808.GPSdata.altitude <= 20){
      
    }
    */

    delay(1);
   }
   
   
