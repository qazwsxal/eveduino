/*
  
 TEST SKETCH, PLEASE IGNORE.
 
 */
#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <TM1638.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01 };
IPAddress ip(192,168,1,20);

// initialize the library instance:
EthernetClient client;

const unsigned long requestInterval = 3600000;  // delay between requests (1 hour, reset the arduino if you're desperate)
/*
Actually, best practice would be to parse the cachedUntil time, but this works well enough.
 */

TM1638 module(8, 9, 7); //module to display countdown on

char serverName[] = "api.eveonline.com"; 

boolean requested;                   // whether you've made a request since connecting
String currentLine = "";            // string to hold the text from server
String Time = "";                  // string to hold the date and time
boolean reading = false;       // if you're currently reading xml
boolean justStarted = true;
boolean timeCorrect = true;
boolean readingEndTime;
long timeRemaining;
time_t timeLastCorrect;
int uptimeWhenLastCorrect;
String GET = "GET /char/SkillQueue.xml.aspx?keyID=";
int mSec;
int mMin;
int mHour;
int mDay;
int skillNum = 0;
boolean firstTry = true; //hacky thing preventing reconnect being called before any data is in skillEnd

//EDIT THESE, remember to leave quotes
String APIKey = "APIKey";
String VerifCode = "verfication code goes here";

time_t skillEnd[32]; //room for 30 skills in queue, like hell you're going to have more.

void setup() {
  // reserve space for the strings:
  currentLine.reserve(256);
  Time.reserve(150);
  //set up GET request
  GET += APIKey;
  GET += "&vCode=";
  GET += VerifCode;
  GET += " HTTP/1.1";
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // attempt to connect, and wait a millisecond:
  Serial.println("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac)) {
    // if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP, trying manually");
    Ethernet.begin(mac, ip);
  }
  Serial.print("My address:");
  Serial.println(Ethernet.localIP());
  // connect to Eve:
  connectToEve();
}



void loop()
{
  if (client.connected()) {
    if (client.available()) {
      
      // read incoming bytes:
      char inChar = client.read();
      firstTry = false; 
      // add incoming byte to end of line:
      // Serial.print(inChar);  Remove comment if you want to see what exactly api.eveonline.com is sending you
      currentLine += inChar; 

      // if you get a newline, clear the line:
      if (inChar == '\n') {
        currentLine = "";
      } 
      if ( currentLine.endsWith("endTime=\"") || currentLine.endsWith("<currentTime>") || currentLine.endsWith("<cachedUntil>")) {
        // tweet is beginning. Clear the tweet string:
        reading = true;
        justStarted = true;
        Time = "";
        if ( currentLine.endsWith("endTime=\"") || currentLine.endsWith("<cachedUntil>")) { 
          readingEndTime = true; 
          skillNum++;
        }
        else
        {
          readingEndTime = false;
        }
      } 

      // if you're currently reading the end time of a skill,
      // add them to the time String:
      if (reading) {
        if (inChar != '>' || justStarted) { // justStarted prevents killing it at the first """
          Time += inChar;
          justStarted = false;
        } 
        else {
          // if you got a """ or character,
          // you've reached the end of the time, leaves a load of trailing shit on <currentTime>, still works,
          reading = false;
          calcTime();
          skillEnd[skillNum] = now();
          setTime(timeLastCorrect);
          timeCorrect = true;
          Serial.println(skillEnd[skillNum]);
        }
      }
      if ( currentLine.endsWith("</eveapi>") )
      {
        client.stop();
        skillNum--;
      }
    }   

  }
  else if (now() > skillEnd[skillNum + 1] || skillEnd[1] < now()) {
    // if the cache has been refreshed or
    // a skill has been completed, connect to eve and refresh the queue.
    connectToEve();
  } 
  //magic happened somewhere in this clusterfuck and skillEnd - now() gives you the time in seconds until skill training is complete
  if(!client.available()){
    switch (module.getButtons()) { //there has to be a nicer way of doing this...
    case 0:
      Serial.println(skillEnd[skillNum] - now());
      timeRemaining = (skillEnd[skillNum] - now());
      break;
    case 1:
      Serial.println(skillEnd[1] - now());
      timeRemaining = (skillEnd[1] - now());
      break;
    case 2:
      Serial.println(skillEnd[2] - now());
      timeRemaining = (skillEnd[2] - now());
      break;
    case 4:
      Serial.println(skillEnd[3] - now());
      timeRemaining = (skillEnd[3] - now());
      break;
    case 8:
      Serial.println(skillEnd[4] - now());
      timeRemaining = (skillEnd[4] - now());
      break;
    case 16:
      Serial.println(skillEnd[5] - now());
      timeRemaining = (skillEnd[5] - now());
      break;
    case 32:
      Serial.println(skillEnd[6] - now());
      timeRemaining = (skillEnd[6] - now());
      break;
    case 64:
      Serial.println(skillEnd[7] - now());
      timeRemaining = (skillEnd[7] - now());
      break;
    case 128:
      Serial.println(skillEnd[8] - now());
      timeRemaining = (skillEnd[8] - now());
      break;
    default:
      Serial.println(skillEnd[skillNum] - now());
      timeRemaining = (skillEnd[skillNum] - now());
    }
    mSec = timeRemaining % 60;
    mMin = timeRemaining % 3600 / 60;
    mHour = timeRemaining % 86400 / 3600;
    mDay = timeRemaining / 86400;
    module.setDisplayDigit(byte(mSec % 10),7, false);
    module.setDisplayDigit(byte(mSec / 10),6, false);
    module.setDisplayDigit(byte(mMin % 10),5, true);
    module.setDisplayDigit(byte(mMin / 10),4, false);
    module.setDisplayDigit(byte(mHour % 10),3, true);
    module.setDisplayDigit(byte(mHour / 10),2, false);
    module.setDisplayDigit(byte(mDay % 10),1, true);
    module.setDisplayDigit(byte(mDay / 10),0, false);

    for(int x=0; x < skillNum; x++){
      module.setLED(TM1638_COLOR_GREEN, x);
      Serial.println(skillEnd[x+1]);
    }
    if(timeRemaining < 86400)
    {
      for(int j=7; j >= skillNum; j--)
        module.setLED(TM1638_COLOR_RED, j);
    }
    else
    {
      for(int j=7; j >= skillNum; j--)
        module.setLED(0, j);      
    }
    if(timeRemaining < 10800)
    {
      delay(500);
      module.setLEDs(0x0000);
      delay(500);
    }
    else
    {
      delay(100);
    }
  }
}
void connectToEve() {
  skillNum = 0;
  Serial.println(GET);
  Serial.println("connecting to server...");
  if (client.connect(serverName, 80)){ 
    delay(1);
    Serial.println("making HTTP request...");
    // make HTTP GET request to twitter:
    client.println(GET);
    client.println("HOST: api.eveonline.com");
    client.println();
    Serial.println("made HTTP request...");
  }
}



void calcTime() {
  Serial.println(Time);
  char yearStr[5] = "2000";
  int year;
  char monthStr[3] = "01";
  int month;
  char dayStr[3] = "01";
  int day;
  char hourStr[3] = "01";
  int hour;
  char minuteStr[3] = "01";
  int minute;
  char secondStr[3] = "01";
  int second;
  //funky date/time parsing, 
  for(int x = 1; x < 5; x++) {
    yearStr[x-1] = Time.charAt(x);  
  }          
  year = atoi(yearStr);
  for(int x = 6; x < 8; x++) {
    monthStr[x-6] = Time.charAt(x);  
  }          
  month = atoi(monthStr);
  for(int x = 9; x < 11; x++) {
    dayStr[x-9] = Time.charAt(x);  
  }          
  day = atoi(dayStr);
  for(int x = 12; x < 14; x++) {
    hourStr[x-12] = Time.charAt(x);  
  }          
  hour = atoi(hourStr);
  for(int x = 15; x < 17; x++) {
    minuteStr[x-15] = Time.charAt(x);  
  }          
  minute = atoi(minuteStr);
  for(int x = 18; x < 20; x++) {
    secondStr[x-18] = Time.charAt(x);  
  }          
  second = atoi(secondStr);
  setTime(hour,minute,second,day,month,year);

  Serial.println(now());
  if(!readingEndTime)
  {
    timeLastCorrect = now();
  }
  Serial.print("Time Last correct: ");
  Serial.println(timeLastCorrect);
}







