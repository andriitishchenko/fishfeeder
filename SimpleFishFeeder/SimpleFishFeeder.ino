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
int buttonState = 0;

void setup() {
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);
  if (! _clock.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
delay(1000);
  if (! _clock.isrunning()) {
    Serial.println("RTC is NOT running!");
    _clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
  }

  //  _servo.attach(servoPin, servoMinImp, servoMaxImp);
  _servo.attach(servoPin);
  delay(1000);
  //_servo.writeMicroseconds(servoMinImp);
  _servo.write(0);
  //servoClose();

}

void servoOpen() {
  Serial.println("ROTATE_MAX");
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
  Serial.println("ROTATE_MIN");
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
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    showTime();
    checkFeed();
    while (Serial.available() > 0)
    {
      char aChar = Serial.read();
      if (aChar == '\n')
      {
        activeIndex = 0;
        TimeStruct* ts = &_feedTimeList[activeIndex];
        ts->feedProgress = ts->feedSize;
        break;
      }
    }

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
  //  return;
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
  Serial.println("FEED NOW");
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


