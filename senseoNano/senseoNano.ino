// Libs

// Pins

int mainLedPin = 2;
int button1Pin = 4;

// Status vars
volatile byte mainLedState;  // 0 = off, 1 = ready (on), 2 = heating (slow), 3/4 = no water (fast)

// Counters
volatile unsigned int pulse;

// Timers
unsigned long readMainLedPulseTimer;
unsigned long otherTimer;

// Intervals
unsigned long readMainLedPulseTimerInterval;
unsigned long otherTimerInterval;

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
  pinMode(button1Pin, OUTPUT);

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
    digitalWrite(button1Pin, HIGH);
    command = "";
  }
  if (command.equalsIgnoreCase("off")) {
    digitalWrite(button1Pin, LOW);
    command = "";
  }
  if (command.equalsIgnoreCase("push")) {
    digitalWrite(button1Pin, HIGH);
    delay(200); // EVIL!
    digitalWrite(button1Pin, LOW);
    command = "";
  }
  if (command.equalsIgnoreCase("status")) {
    Serial.print("State is: ");
    Serial.println(mainLedState);
    command = "";
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
