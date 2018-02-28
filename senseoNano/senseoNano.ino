// Libs
#include "OneButton.h"

// Pins

int mainLedPin = 2;
int buttonSmallPushPin = 4;
int buttonSmallReadPin = 5;
int buttonPowerPushPin = 6;
int buttonPowerReadPin = 7;
int buttonLargePushPin = 8;
int buttonLargeReadPin = 11;

int rgbGreenPin = 9;
int rgbBluePin = 10;
int rgbRedPin = 12;

int cupSensorPin = A2;
int lidSensorPin = A4;
int padSensorPin = A6;

// Sensor status vars
// holds the current state of the senseo (0 = off, 1=ready, 2=heating/making (slow blink), 3 = no water (fast blinking)
volatile byte mainLedState;
// holds the state of the pad sensor (true = old pad, false=fresh pad)
volatile boolean padUsed;
// holds the state of the lid sensor (true = closed, false = open)
volatile boolean lidClosed;
// is a cup present ( 0 = no, 1 = yes)
volatile boolean cupStatus;
// commands
volatile boolean autoSmall;     //autostart for one cup
volatile boolean autoNetSmall;  //autostart for one cup comming from network
volatile boolean autoLarge;     //autostart for two cups
volatile boolean autoNetLarge;  //autostart for two cups comming from network

// utillities LED blinking
volatile boolean mainLedOff;    //mainLed on or off for timing of second Led

// utillities: Timers/Counters/Intervals
unsigned long readMainLedPulseTimer;
unsigned long readMainLedPulseTimerInterval;

unsigned long ledLastOff = 0UL;
unsigned long lastLedSwitch;

volatile unsigned int pulse;
volatile unsigned int ledBlinkCount;
// for switching on/off green/blue
volatile boolean green;
volatile boolean blue;

// mainLED pulse threshold
int mainLedThreshold;
int padResistanceThreshold;
int cupWeightThreshold;

// Serial Communication
String command; // String input from command prompt
char inByte; // Byte input from command prompt

// Buttons
OneButton smallButton(buttonSmallReadPin, false);
OneButton largeButton(buttonLargeReadPin, false);
OneButton powerButton(buttonPowerReadPin, false); //TODO: needed?

void setup() {
  // Set Serial speed
  Serial.begin(115200);

    // Set thresholds
  mainLedThreshold = 10;   // below 10 per 2 seconds = slow, greater 10 per 2 seconds = fast
  padResistanceThreshold = 1000000; // if resistance of pad is below value it is wet --> used
  cupWeightThreshold = 100; //TODO
  // Set Intervals
  readMainLedPulseTimerInterval = 2000UL;   // count pulses for 2 seconds

  // Initialize Timers
  readMainLedPulseTimer = millis();

  // Configure interrup to read the LED pulse
  //pinMode(mainLedPin, INPUT_PULLUP);
  pinMode(mainLedPin, INPUT); // Connected with pulldown
  attachInterrupt(digitalPinToInterrupt(mainLedPin), count_main_led_pulse, CHANGE);


  // Configre sensors
  pinMode(lidSensorPin, INPUT);
  pinMode(padSensorPin, INPUT_PULLUP);
  //pinMode Pad-Sensor = Analog

  // Configure Buttons
  pinMode(buttonSmallPushPin, OUTPUT);
  pinMode(buttonPowerPushPin, OUTPUT);
  pinMode(buttonLargePushPin, OUTPUT);

  pinMode(buttonSmallReadPin, INPUT);
  pinMode(buttonPowerReadPin, INPUT);
  pinMode(buttonLargeReadPin, INPUT);

  //Configure RGB-LED
  pinMode(rgbRedPin, OUTPUT);
  pinMode(rgbGreenPin, OUTPUT);
  pinMode(rgbBluePin, OUTPUT);

  smallButton.attachClick(smallClick);
  smallButton.attachDoubleClick(smallDoubleClick);
  largeButton.attachClick(largeClick);
  largeButton.attachDoubleClick(largeDoubleClick);
  powerButton.attachClick(powerClick);

  // initialize green/blue as off
  green=false;
  blue=false;

}

// is a used pad in the machine?
//TODO: set the current voltage as reference!
boolean getPadUsedStatus() {
  // This function uses a Voltage Devider with a 27k Ohm resistor to measure the resistance of the coffee pad
  // experiments have shown:
  // the resistance of a dry pad is >10M Om
  // the resistance of a wet pad is < 100k Ohm (pad is used)

  // fancy formula from: https://arduino.stackexchange.com/questions/28222/a-question-about-resistance-measurement-with-arduino

  // get raw sensor value
  delay(20);
  int sensorValue = analogRead(padSensorPin);
  delay(20);
  // calculate the voltage dropped by the unknown resistor
  float dv = (sensorValue / 1024.0) * 5.0;
  // with the dropped voltage dv, the known 27k Ohm of the first resistor and the knowledge of having 5V from the Arduino we can calulate the resistance of the coffee pad
  float res = 27000.0 * (1 / ((5.0 / dv) - 1));
  
  //if debug
  Serial.print("Resistance is: ");
  Serial.println(res);

  // if resistance is lower 1M the pad is used
  if (res < padResistanceThreshold) {
    padUsed = true;
  }
  else {
    padUsed = false;
  }
  return padUsed;
}

// get status of main LED
byte getMainLedStatus() {
  // debug only
  Serial.print("Pulse: ");
  Serial.println(pulse);

  // If there is no pulse: Is the LED on or off?
  if (pulse == 0) {
    mainLedState = digitalRead(mainLedPin); // 0 = off, 1 = ready (on), 2 = heating, 3 = no water
  }
  // pulse is slow
  else if (pulse > 0 && pulse < mainLedThreshold) {
    mainLedState = 2; // slow/heating
  }
  // pulse is fast
  else if (pulse >= mainLedThreshold) {
    mainLedState = 3; // fast/no water
  }
  // reset pulse
  pulse = 0;

  // reset timer
  readMainLedPulseTimer = millis();

  return mainLedState;
}

// Interrupt callback to count LED pulses
void count_main_led_pulse()
{
  // count pulse
  pulse++;

  // reset blue or green on every change of red
  green = false;
  blue = false;
  digitalWrite(rgbBluePin,LOW);
  digitalWrite(rgbGreenPin,LOW);
  
  // if changed to "on"
  if (digitalRead(mainLedPin)) {

    // debug only (set second led to same state as main LED)
    digitalWrite(rgbRedPin, HIGH);

    // state of main LED
    mainLedOff = false;
    
    // reset blink count (blink of blue or green)
    ledBlinkCount = 0;
  }
  else {
    // timing for second LED
    ledLastOff = millis();
    // state of main LED
    mainLedOff = true;

    // debug only (set second led to same state as main LED)
    digitalWrite(rgbRedPin, LOW);
    
  }

}

// recognize power toggle
void powerClick() {

  //TODO: Could be wrong value if mainLedState is not already updated
  if(mainLedState == 0){
    Serial.println("s:power toggle - now off");
  }
  else {
    Serial.println("s:power toggle - now on");
  }
}
// report manual small click
void smallClick() {
  Serial.println("s:manual small click");
}
void largeClick() {
  Serial.println("s:manual large  click");
}
// report auto small and enable auto small
void smallDoubleClick() {
  Serial.println("s:auto small");
  autoSmall = true;
  autoLarge = false;
  autoNetSmall = false;
  autoNetLarge = false;
}
void largeDoubleClick() {
  Serial.println("s:auto large");
  autoSmall = false;
  autoLarge = true;
  autoNetSmall = false;
  autoNetLarge = false;
}
// report auto small and enable auto small
void smallNet() {
  Serial.println("s:network auto small");
  autoSmall = false;
  autoLarge = false;
  autoNetSmall = true;
  autoNetLarge = false;
}
void largeNet() {
  Serial.println("s:network auto large");
  autoSmall = false;
  autoLarge = false;
  autoNetSmall = false;
  autoNetLarge = true;
}

void togglePower() {
  digitalWrite(buttonPowerPushPin, HIGH);
  delay(30);
  digitalWrite(buttonPowerPushPin, LOW);
}

void pushSmall() {
  digitalWrite(buttonSmallPushPin, HIGH);
  delay(30);
  digitalWrite(buttonSmallPushPin, LOW);
}
void pushLarge() {
  digitalWrite(buttonLargePushPin, HIGH);
  delay(30);
  digitalWrite(buttonLargePushPin, LOW);
}

boolean getLidStatus() {
  lidClosed = digitalRead(lidSensorPin);
  return lidClosed;
}
boolean getCupStatus() {
  //TODO: read cup sensor
  int value=500;
  if (value > cupWeightThreshold){
    cupStatus = true;
    return true;
  }
  else {
    cupStatus = false;
    return false;
  }
}
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////

// Handle serial commands (comming from ESP (network)
void handleSerial() {
  // if input at serial is available read it
  if (Serial.available() > 0) {
    inByte = Serial.read();
    // only input if a letter, number, =,?,+ are typed!
    if ((inByte >= 65 && inByte <= 90) || (inByte >= 97 && inByte <= 122) || (inByte >= 48 &&     inByte <= 57) || inByte == 43 || inByte == 61 || inByte == 63) {
      command.concat(inByte);
    }
  }
  //newline
  if (inByte == 10 || inByte == 13) {

    inByte = 0;
  }
  
  // makeSmall from network
  if (command.equalsIgnoreCase("makeSmall")) {
    // already busy
    if (autoLarge || autoSmall) {
      Serial.println("e:busy");
    }
    else {
      smallNet();
    }
    
    // clear command
    command = "";
  }

  // makeLarge from network
  if (command.equalsIgnoreCase("makeLarge")) {
    // already busy
    if (autoLarge || autoSmall) {
      Serial.println("e:busy");
    }
    else {
      largeNet();
    }

    // clear command
    command = "";
  }
  if (command.equalsIgnoreCase("status")) {
    // check lid
    // check pad
    // check status (off, no water, heating, ready)
    // refresh all readings
    getLidStatus();
    getPadUsedStatus();
    getMainLedStatus();
    Serial.print("lidClosed:");
    Serial.print(lidClosed);
    Serial.print(",padUsed:");
    Serial.print(padUsed);
    Serial.print(",status:");
    Serial.println(mainLedState);

    // clear command
    command = "";
  }
  

}
//////////////////////////////////////////////////////
//////////////////////////////////////////////////////
void loop() {
  // tick the buttons
  smallButton.tick();
  largeButton.tick();
  powerButton.tick();

  // refresh status (every X seconds, X=readMainLedPulseTimerInterval)
  getLidStatus();
  getPadUsedStatus();
  getCupStatus();
  if ((millis() - readMainLedPulseTimer) > readMainLedPulseTimerInterval)
  {
    getMainLedStatus();
    
  }

  // turn on for task
  if ((autoSmall || autoNetSmall || autoLarge || autoNetLarge) && mainLedState == 0){
    togglePower();
  }

  

}
