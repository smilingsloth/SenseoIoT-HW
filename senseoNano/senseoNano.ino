// Libs

// Pins

int mainLedPin = 2;
int buttonPushSmallPin = 4;
int buttonPushPowerTriggerPin =  5;
int buttonPushLargePin = 6;

//int buttonReadSmallPin = 1000;

int padSensorPin=A7;
// Triggers
boolean readPad;
boolean readLed;
//boolean readLid;

// Status vars
volatile byte mainLedState;  // 0 = off, 1 = ready (on), 2 = heating (slow), 3/4 = no water (fast)
volatile boolean padUsed;
volatile boolean lidClosed;

// Counters
volatile unsigned int pulse;

// Timers
unsigned long readMainLedPulseTimer;
unsigned long padUsedTimer;

// Intervals
unsigned long readMainLedPulseTimerInterval;
unsigned long padUsedInterval;

// mainLED pulse threshold
int mainLedThreshold;

// Serial Communication
String command; // String input from command prompt
char inByte; // Byte input from command prompt

// Function definitions
void count_pulse();
void handleSerial();

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
  pinMode(mainLedPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(mainLedPin), count_main_led_pulse, CHANGE);

  // Configure Buttons
  pinMode(buttonPushSmallPin, OUTPUT);

}

void loop() {
  // Check for serial command
  handleSerial();

  // read main LED
  if ((millis() - readMainLedPulseTimer) > readMainLedPulseTimerInterval)
  {
    getMainLedState();
  }

  

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
  if (command.equalsIgnoreCase("on")) {
    digitalWrite(buttonPushSmallPin, HIGH);
    command = "";
  }
  if (command.equalsIgnoreCase("off")) {
    digitalWrite(buttonPushSmallPin, LOW);
    command = "";
  }
  if (command.equalsIgnoreCase("push")) {
    digitalWrite(buttonPushSmallPin, HIGH);
    delay(200); // EVIL!
    digitalWrite(buttonPushSmallPin, LOW);
    command = "";
  }
  if (command.equalsIgnoreCase("status")) {
    Serial.print("State is: ");
    Serial.println(mainLedState);
    command = "";
  }
}

// is a used pad in the machine?
void getPadUsedStatus() {
  // This function uses a Voltage Devider with a 27k Ohm resistor to measure the resistance of the coffee pad
  // experiments have shown:
  // the resistance of a dry pad is >10M Om
  // the resistance of a wet pad is < 100k Ohm (pad is used)
  
  // fancy formula from: https://arduino.stackexchange.com/questions/28222/a-question-about-resistance-measurement-with-arduino

  // get raw sensor value
  int sensorValue = analogRead(padSensorPin);
  // calculate the voltage dropped by the unknown resistor
  float dv = (sensorValue / 1024.0)*5.0;
  // with the dropped voltage dv, the known 27k Ohm of the first resistor and the knowledge of having 5V from the Arduino we can calulate the resistance of the coffee pad
  float res = 27000.0 * (1/((5.0/dv)-1));
  //Serial.print("Resistance is: ");
  //Serial.println(res);

  // if resistance is lower 100k the pad is used
  if(res < 100000){
    padUsed = true;
  }
  else {
    padUsed = false;
  }
}

byte getMainLedState() {
  //Serial.print("Pulse: ");
  //Serial.println(pulse);

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
}

// Interrupt callback to count LED pulses
void count_main_led_pulse()
{
  pulse++;
}
