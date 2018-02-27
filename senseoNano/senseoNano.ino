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



// Status vars
volatile byte mainLedState;  // 0 = off, 1 = ready (on), 2 = heating (slow), 3/4 = no water (fast)
volatile boolean padUsed;
volatile boolean lidClosed;
boolean autoSmall = false;
boolean autoNetSmall = false;
boolean autoLarge = false;
boolean autoNetLarge = false;
boolean mainLedOff = false;
boolean green = false;
boolean blue = false;

// Counters
volatile unsigned int pulse;
unsigned int ledBlinkCount;
// Timers
unsigned long readMainLedPulseTimer;

unsigned long ledLastOff = 0UL;
unsigned long lastLedSwitch;

OneButton smallButton(buttonSmallReadPin, false);
OneButton largeButton(buttonLargeReadPin, false);

// Intervals
unsigned long readMainLedPulseTimerInterval;


// mainLED pulse threshold
int mainLedThreshold;

// Serial Communication
String command; // String input from command prompt
char inByte; // Byte input from command prompto

// Function definitions
//void count_pulse();
//void handleSerial();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Set thresholds
  mainLedThreshold = 10;   // below 10 per 2 seconds = slow, greater 10 per 2 seconds = fast

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

}

void loop() {
  smallButton.tick();
  largeButton.tick();
  // Check for serial command
  handleSerial();

  // read main LED + status
  if ((millis() - readMainLedPulseTimer) > readMainLedPulseTimerInterval)
  {
    getMainLedStatus();
    getLidStatus();
    getPadUsedStatus();

  }
  // turn on for task
  if ((autoSmall || autoNetSmall || autoLarge || autoNetLarge) && mainLedState == 0){
    togglePower();
  }
  // blink green/blue
  if (mainLedState != 2) {
    green = false;
    digitalWrite(rgbGreenPin, green);
    blue = false;
    digitalWrite(rgbBluePin, blue);

  }
  // if autostart one cup and state is heating
  if (autoSmall && mainLedState == 2 && (millis() - lastLedSwitch) > 100 && ledBlinkCount < 2 && mainLedOff) {
    //
    green = !green;
    digitalWrite(rgbGreenPin, green);
    ledBlinkCount ++;
    lastLedSwitch = millis();
  }
  if (autoLarge && mainLedState == 2 && (millis() - lastLedSwitch) > 100 && ledBlinkCount < 4 && mainLedOff) {
    //
    green = !green;
    digitalWrite(rgbGreenPin, green);
    ledBlinkCount ++;
    lastLedSwitch = millis();
  }
  if (autoNetSmall && mainLedState == 2 && (millis() - lastLedSwitch) > 100 && ledBlinkCount < 2 && mainLedOff) {
    //
    blue = !blue;
    digitalWrite(rgbBluePin, blue);
    ledBlinkCount ++;
    lastLedSwitch = millis();
  }
  if (autoNetLarge && mainLedState == 2 && (millis() - lastLedSwitch) > 100 && ledBlinkCount < 4 && mainLedOff) {
    //
    blue = !blue;
    digitalWrite(rgbBluePin, blue);
    ledBlinkCount ++;
    lastLedSwitch = millis();
  }

  if ((autoSmall || autoNetSmall) && mainLedState == 1) {
    delay(1000);
    pushSmall();
    Serial.println("s:made small");
    autoSmall = false;
    autoNetSmall = false;
  }
  if ((autoLarge || autoNetLarge) && mainLedState == 1) {
    delay(1000);
    pushLarge();
    Serial.println("s:made large");
    autoLarge = false;
    autoNetLarge = false;
  }
  if (autoSmall && mainLedState == 3) {
    Serial.println("e:aborted autoSmall, no water");
    autoSmall = false;
  }
  if (autoNetSmall && mainLedState == 3) {
    Serial.println("e:aborted autoNetSmall, no water");
    autoNetSmall = false;
  }
  if (autoLarge && mainLedState == 3) {
    Serial.println("e:aborted autoLarge, no water");
    autoLarge = false;
  }
  if (autoNetLarge && mainLedState == 3) {
    Serial.println("e:aborted autoNetLarge, no water");
    autoNetLarge = false;
  }
  ///////////////////////////////
  if (autoSmall && lidClosed == false) {
    Serial.println("e:aborted autoSmall, lid open");
    autoSmall = false;
  }
  if (autoNetSmall && lidClosed == false) {
    Serial.println("e:aborted autoNetSmall, lid open");
    autoNetSmall = false;
  }
  if (autoLarge && lidClosed == false) {
    Serial.println("e:aborted autoLarge, lid open");
    autoLarge = false;
  }
  if (autoNetLarge && lidClosed == false) {
    Serial.println("e:aborted autoNetLarge, lid open");
    autoNetLarge = false;
  }
  ////////////////////////////////////////
  if (autoSmall && padUsed) {
    Serial.println("e:aborted autoSmall, pad already used");
    autoSmall = false;
  }
  if (autoNetSmall && padUsed) {
    Serial.println("e:aborted autoNetSmall, pad already used");
    autoNetSmall = false;
  }
  if (autoLarge && padUsed) {
    Serial.println("e:aborted autoLarge, pad already used");
    autoLarge = false;
  }
  if (autoNetLarge && padUsed) {
    Serial.println("e:aborted autoNetLarge, pad already used");
    autoNetLarge = false;
  }


  delay(10);


}
void smallClick() {
  Serial.println("s:manual small click");
}

void smallDoubleClick() {
  Serial.println("s:auto small");
  autoSmall = true;
}
void largeClick() {
  Serial.println("s:manual large  click");
}
void largeDoubleClick() {
  Serial.println("s:auto large");
  autoLarge = true;
}

// Handle serial commands
void handleSerial() {
  if (Serial.available() > 0) {
    inByte = Serial.read();
    // only input if a letter, number, =,?,+ are typed!
    if ((inByte >= 65 && inByte <= 90) || (inByte >= 97 && inByte <= 122) || (inByte >= 48 &&     inByte <= 57) || inByte == 43 || inByte == 61 || inByte == 63) {
      command.concat(inByte);
    }
  }
  if (inByte == 10 || inByte == 13) {

    inByte = 0;
  }
  // new commands
  if (command.equalsIgnoreCase("makeSmall")) {
    if (autoLarge || autoSmall) {
      Serial.println("e:busy");
    }
    else {
      autoNetSmall = true;
    }
    

    // clear command
    command = "";
  }
  if (command.equalsIgnoreCase("makeLarge")) {
    if (autoLarge || autoSmall) {
      Serial.println("e:busy");
    }
    else {
      autoNetLarge = true;
    }

    // clear command
    command = "";
  }
  if (command.equalsIgnoreCase("status")) {
    // check lid
    // check pad
    // check status (off, no water, heating, ready)
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
// is a used pad in the machine?
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
  Serial.print("Resistance is: ");
  Serial.println(res);

  // if resistance is lower 1M the pad is used
  if (res < 1000000) {
    padUsed = true;
  }
  else {
    padUsed = false;
  }
  return padUsed;
}

byte getMainLedStatus() {
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
  pulse++;
  if (digitalRead(mainLedPin)) {
    
    digitalWrite(rgbRedPin, HIGH);
    mainLedOff = false;
    ledBlinkCount = 0;
    green = false;
    blue = false;
  }
  else {
    ledLastOff = millis();
    mainLedOff = true;

    digitalWrite(rgbRedPin, LOW);
    green = false;
    blue = false;
  }

}


