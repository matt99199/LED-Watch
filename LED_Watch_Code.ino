//Matt McCormick
//8/24/2017
//LED Watch Code Rev 0.1
//Notes: Add effects and dimming to level display
//       Look into changing/adding level/digit effects

#include <Wire.h>
#include <DS3231.h>
#include <BH1750.h>
#include <avr/sleep.h>

int allOutputs[16] = {0, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, A1, A2, A3};
int segmentOutputs[10] = {A3, A2, A1, 13, 12, 8, 7, 4, 1, 0};
int lineOutputs1[6] = {6, 5, 11, 10, 9, 3};
int lineOutputs2[6] = {3, 9, 10, 11, 5, 6};
int digitOutputs[2][60];
int digitMemory[2];
int intensityDivConst = 5;
int hour, minute, second, buttonVal, intensity, swDelay, lastMode, lightLevel, batteryPerc, temperatureLevel;
int intensityThresh = 2000;
int maxDim = 200;
int minDim = 10;
int modeCount = 12;
int settingCount = 8;
int swLockLimit = 100;
int mode = 0;
bool maxDimLatch = false;
bool swLock = false;
bool editSettings = false;
bool dum1, dum2;
long vcc;
float temperature;

DS3231 Clock;
BH1750 lightMeter;

void setup() {
  blankDisplay();
  lightMeter.begin(BH1750_CONTINUOUS_LOW_RES_MODE);
  attachInterrupt(digitalPinToInterrupt(2), wake, CHANGE);
  for (int i, j, k = 0; i < 60; i++) {
    k = i % 6;
    j = i / 6;
    digitOutputs[0][i] = segmentOutputs[j];
    if (j % 2) digitOutputs[1][i] = lineOutputs2[k];
    else digitOutputs[1][i] = lineOutputs1[k];
  }
}

void loop() {
  //Check Light Sensor Level
  if ( maxDimLatch || (intensity > intensityThresh) ) intensity = maxDim;
  else intensity = map(lightMeter.readLightLevel(), 0, intensityThresh, minDim, maxDim);
  //Check button status
  buttonVal = analogRead(6);
  if ( (buttonVal < 1000)  && !swLock) {
    swLock = !swLock;
    blankDisplay();
    if ( (buttonVal >= 800) && (buttonVal < 1000) ) mode++;
    else if ( (buttonVal >= 500) && (buttonVal < 800) ) mode--;
    else if ( (buttonVal >= 300) && (buttonVal < 500) ) {
      editSettings = !editSettings;
      if (editSettings) {
        lastMode = mode;
        mode = 0;
      }
      else mode = lastMode;
    }
  }
  //Time lock out switch after press
  if ( (buttonVal >= 1000) && swLock ) {
    swDelay++;
    if ( swDelay >= swLockLimit ) swLock = !swLock;
  }
  //Switch based on mode or editSettings state
  if (editSettings) {
    switch (mode % settingCount) {
      case 0: //Set time
        //Clock.setHour(Hour);
        //Clock.setMinute(Minute);
        //Clock.setSecond(Second);
        break;
      case 1: //Set date
        break;
      case 2: //Set mindim value
        break;
      case 3: //Toggle maxdim
        break;
      case 4: //Set sleep timeout
        break;
      case 5: //Set accelerometer interrupt function
        break;
      case 6: //Set major LED division constant
        break;
      case 7: //Set active interrupt mode
        break;
    }
  }
  else switch (mode % modeCount) {
      case 0: //Watch
        //Get RTC Information
        hour = (Clock.getHour(dum1, dum2) % 12) * 5;
        minute = Clock.getMinute();
        second = Clock.getSecond();
        //Display RTC Information
        digitDisplay(1, hour, intensity);
        digitDisplay(0, minute, intensity);
        digitDisplay(1, second, intensity);
        digitDisplay(0, second, intensity);
        break;
      case 1: //Thermometer
        temperature = Clock.getTemperature();
        if ( temperature >= 0 ) {
          temperatureLevel = map(temperature, 0, 40, 1, 59);
          levelDisplay(1, temperatureLevel, intensity);
        }
        else {
          temperatureLevel = map(temperature, -20, -1, 40, 59);
          levelDisplay(0, temperatureLevel, intensity);
        }
        break;
      case 2: //Accelerometer
        //TBD
        break;
      case 3: //Speedometer
        //TBD
        break;
      case 4: //Light level
        lightLevel = map(lightMeter.readLightLevel(), 50, 3000, 0, 59);
        levelDisplay(1, lightLevel, intensity);
        break;
      case 5: //Battery level
        batteryPerc = map(readVcc(), 2750, 4200, 0, 59);
        levelDisplay(2, batteryPerc, intensity);
        break;
      case 6: //Flash light major color
        levelDisplay(1, 59, maxDim);
        break;
      case 7: //Flash light minor color
        levelDisplay(0, 59, maxDim);
        break;
      case 8: //Hazard indicator
        break;
      case 9: //Calendar
        break;
      case 10: //Demo mode
        break;
      case 11: //Rave mode (place holder if nothing else)
        break;
    }
  //Sleep after timeout is reached
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_mode();
}

void blankDisplay() {
  for (int i = 0; i < 16; i++) pinMode(allOutputs[i], INPUT);
}

void digitDisplay(int dir, int num, int intn) {
  if (num % 5 == 0) intn = intn / intensityDivConst;
  if (digitOutputs[0][num] != digitMemory[0]) pinMode(digitMemory[0], INPUT);
  if (digitOutputs[1][num] != digitMemory[1]) pinMode(digitMemory[1], INPUT);
  pinMode(digitOutputs[0][num], OUTPUT);
  pinMode(digitOutputs[1][num], OUTPUT);
  digitalWrite(digitOutputs[0][num] , dir);
  for (int i = 0; i < intn; i++) digitalWrite(digitOutputs[1][num], !dir);
  for (int i = 0; i <= (maxDim - intn); i++) pinMode(digitOutputs[1][num], INPUT);
  digitMemory[0] = digitOutputs[0][num];
  digitMemory[1] = digitOutputs[1][num];
}

void levelDisplay(int dir, int num, int intn) {
  if (dir == 2) {
    //special shit for battery level
  }
  for ( int i = 0; i < 5; i++ ) {
    pinMode(digitOutputs[0][i], OUTPUT);
    digitalWrite(digitOutputs[0][i], dir);
  }
  for ( int i = 0; i < (num % 6); i++ ) {
    if ( i == num % 6 ) {
      for ( int i = 1; i < (6 - (num - (i * 6))); i++) {
        pinMode(digitOutputs[0][num + i], INPUT);
      }
    }
    pinMode(digitOutputs[1][(i * 6) - 6], INPUT);
    pinMode(digitOutputs[1][i * 6], OUTPUT);
    digitalWrite(digitOutputs[1][i * 6], !dir);
  }
}

void wake() {
  blankDisplay();
}

long readVcc() {
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(1); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA, ADSC));
  vcc = ADCL;
  vcc |= ADCH << 8;
  vcc = 1126400L / vcc; // Back-calculate AVcc in mV
  return vcc;
}
