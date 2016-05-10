/*
   Feeder for fish

   Used:
   - RTC Timer
   - Servo SG90
   - arduino Pro mini 326p, 5v

   To add feed times edit _feedTimeList array,
   _feedTimeList[0] uses for manual feeding, so not change it
*/

#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>

const int servoPin = 9;
const int buttonPin = 2;
const int ledPin = 13; //debug

Servo _servo;
RTC_DS1307 _clock;

typedef struct TimeStruct {
  byte hour;
  byte minute;
  byte feedSize;    //count of servo moves, need select depends on holes size in feeder
  byte feedProgress;
} TimeStruct;

TimeStruct _feedTimeList[] = {
  {24, 0, 5, 0}, //on serial read, manual feed,!! DONT REMOVE !!
  {8, 0, 5, 0}, //alarm
  {20, 0, 5, 0}  //alarm ...
};

//helper vars
unsigned long previousMillis = 0;
const long interval = 1000;
byte activeIndex = 100;
//unsigned long previousMillisRotate = 0;
//const long intervalRotate = 5000;
unsigned long previousMillisFeed = 0;
const long servoSpeed = 7; //speed of servo
int isBusy = 0;
int isTimerReady = 0;
int buttonState = 0;
DateTime _datetime;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);//debug
  _servo.attach(servoPin);

  
  if (! _clock.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  delay(1000);
  while (! _clock.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    _clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    delay(3000);
    blinkStatus(5);
  }

  if (_clock.isrunning()) {
    delay(1000);
    //_servo.writeMicroseconds(servoMinImp);
    _servo.write(0);
    //servoClose();
    blinkStatus(3);
    isTimerReady = 1;
  }


  //  _servo.attach(servoPin, servoMinImp, servoMaxImp);



}

void blinkStatus(int code) {
  digitalWrite(ledPin, LOW);
  for (int i = 0; i <= code; i++)
  {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
  }
}

void servoOpen() {
  Serial.println(F("ROTATE_MAX"));
  isBusy = 1;
  for (int servoAngle = 0; servoAngle < 180; servoAngle++) //move the micro servo from 0 degrees to 180 degrees
  {
    _servo.write(servoAngle);
    delay(servoSpeed);
  }
  isBusy = 0;
}

void servoClose()
{
  Serial.println(F("ROTATE_MIN"));
  isBusy = 1;
  int servoAngle = _servo.read();
  if (servoAngle == 0) {
    isBusy = 0;
    return;
  }
  for (servoAngle; servoAngle > 0; servoAngle--) //now move back the micro servo from 0 degrees to 180 degrees
  {
    _servo.write(servoAngle);
    delay(servoSpeed);
  }
  isBusy = 0;
}

void loop() {
  if (isTimerReady == 0) return;
  while(Serial.available()) {
    String inputStr = Serial.readString();
    char charBuf[50];
    inputStr.toCharArray(charBuf, 20) ;//2016/05/09 23:30:00
    if(getTime(charBuf))
    {
      _clock.adjust(_datetime);
    }

  //
  float batteryVoltageRead = analogRead (A0);
  float batteryVoltage =float( batteryVoltageRead * (5/1023.));
  Serial.println (batteryVoltage);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    showTime();
    checkFeed();
    //    while (Serial.available() > 0)
    //    {
    //      char aChar = Serial.read();
    //      if (aChar == '\n')
    //      {
    //        activeIndex = 0;
    //        TimeStruct* ts = &_feedTimeList[activeIndex];
    //        ts->feedProgress = ts->feedSize;
    //        break;
    //      }
    //    }

    buttonState = digitalRead(buttonPin);

    if (buttonState == HIGH) {
      activeIndex = 0;
      TimeStruct* ts = &_feedTimeList[activeIndex];
      ts->feedProgress = ts->feedSize;
    }
  }

  if (activeIndex != 100) {
    feedNow(activeIndex);
  }

}

void showTime() {
  //return;
  DateTime now = _clock.now();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void feedNow(byte index) {
  if (isBusy == 1) return;
  //  unsigned long currentMillis = millis();
  //  if (currentMillis - previousMillisRotate >= intervalRotate) {
  Serial.println(F("FEED NOW"));
  //    previousMillisRotate = currentMillis;
  TimeStruct* ts = &_feedTimeList[index];
  if (ts->feedProgress > 0) {

    if (ts->feedProgress % 2 == 0) {
      //        _servo.writeMicroseconds(servoMinImp);
      //        _servo.write(0);
      servoClose();
    } else
    {
      //        _servo.writeMicroseconds(servoMaxImp);
      //        _servo.write(180);
      servoOpen();
    }
    ts->feedProgress --;
  }

  else {
    activeIndex = 100;
    //    _servo.writeMicroseconds(servoMinImp);
    servoClose();
  }
  //  }
}

void checkFeed() {
  DateTime now = _clock.now();
  byte _count = sizeof( _feedTimeList ) / sizeof( _feedTimeList[0] );

  for (byte i = 0; i < _count; i++) {
    TimeStruct* ts = &_feedTimeList[i];
    if (ts->hour == now.hour() && ts->minute == now.minute() && now.second() == 0) {
      ts->feedProgress = ts->feedSize;
      activeIndex = i;
    }
  }

  //debug
  //  if (activeIndex == 100)
  //  {
  //    if (now.second() == 0) {
  //      activeIndex = 0;
  //      TimeStruct* ts = &_feedTimeList[activeIndex];
  //      ts->feedProgress = ts->feedSize;
  //    }
  //  }
}

bool getTime(const char *str)
{
  int yyyy,mm,d,h,m,s;

   if (sscanf(str, "%d/%d/%d %d:%d:%d",&yyyy,&mm, &d, &h, &m, &s) != 6) return false;
   _datetime = DateTime(yyyy,mm,d,h,m,s);
    return true;
}

//bool getDate(const char *str)
//{
//  char Month[12];
//  int Day, Year;
//  uint8_t monthIndex;
//
//  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
//  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
//    if (strcmp(Month, monthName[monthIndex]) == 0) break;
//  }
//  if (monthIndex >= 12) return false;
//  tm.Day = Day;
//  tm.Month = monthIndex + 1;
//  tm.Year = CalendarYrToTm(Year);
//  return true;
//}


