/**********************************************************************************************************************
  Mailbox Notifier - Mailbox Side

  This sketch was written to work as a Snail mail delivery notification system. It uses two LEDs mounted to the mailbox
  as local (to the mailbox) delivery notification. It also sends out delivery/retrieve data via an XBee modem (serial). When
  the receiving XBee (set in a remote location, like your house) receives the data it turns a delivery indicator
  light on or off and records and displays the delivery/retrieve data.

  For more information see - http://adambyers.com/2013/11/mailbox-notifier/ or ping adam@adambyers.com

  All code (except external libraries and third party code) is published under the MIT License.

**********************************************************************************************************************/

// Libraries
#include "avr/sleep.h"
#include "avr/interrupt.h"
#include "Wire.h"
#include "OneWire.h"
#include "DallasTemperature.h"

// PIN Names
int deliverSW = 2;
int retrieveSW = 3;
int xbPower = 4;
int notifyLED1 = 5;
int notifyLED2 = 6;
int domeLED = 7;
int battery = A0;

// Misc vars
int notify = 3; // The notify state, 1=delivered 0=retrieved
int xbWakeupDelay = 1000; // Number of milliseconds we wait after waking the XBee before using it.
int transmitTimes = 4; // How many times we transmit out the delivery/retrieve data. More than once to make sure it's retrieved.
int xbTransmitDelay = 6000; // Number of milliseconds we wait between XBee transmissions, so the transmissions are not stepping on each other.
int battPinReading = 0; // Holds the raw reading from the battery monitor pin.
float battPinVoltage = 0; // Holds the raw pin voltage value.
float battVoltage = 0; // Calculated battery voltage.
float dividerRatio = 2.242; // Voltage divider ratio.
float tempC = 0; // Mailbox temp reading in celsius.
float tempF = 0; // Mailbox temp in fahrenheit.
byte keepADCSRA; // Keeps the value of the analog digital converter so we can re-enable it when waking up.
bool delivery = false;

// DS18B20 setup
#define tempSensor 8 // PIN the DS18B20 is connected to.
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // Pass the OneWire reference to the DS18B20.

void setup() {
  // Set unused digital pins to outputs and set them low to reduce power consumption when awake.
  for (int unusedPINs = 9; unusedPINs < 13; unusedPINs++) {
    pinMode(unusedPINs, OUTPUT);
    digitalWrite(unusedPINs, LOW);
  }

  // Set used PINs as inputs or outputs.
  pinMode(deliverSW, INPUT);
  pinMode(retrieveSW, INPUT);
  pinMode(xbPower, OUTPUT);
  pinMode(notifyLED1, OUTPUT);
  pinMode(notifyLED2, OUTPUT);
  pinMode(domeLED, OUTPUT);
  pinMode(battery, INPUT);

  // Enable internal resistors so we don't have to use pesky external resistors for the switches.
  digitalWrite(deliverSW, HIGH);
  digitalWrite(retrieveSW, HIGH);

  Serial.begin(9600); // Start your serial engines!

  sensors.begin(); // Start the DallasTemperature library.
}

/*
  As soon as the Arduino is powered on it goes into the main loop where it runs the sleep function that puts the XBee to sleep,
  turns off brown-out, turns off the ADC, and then puts itself to sleep until mail is delivered. If the retrieve door is opened
  before mail is delivered the Arduino wakes up, checks to see if 'delivery = true' before it runs the retrieved sequence. If
  the retrieve door is opened and 'delivery = false' then the Arduino goes right back to sleep. This is the same if mail has been
  delivered 'delivery = true' and the delivery door is opened before mail has been retrieved.
*/

void loop() {
  F_sleep(); // Go to sleep.
}

// Functions
void F_sleep(void) {
  attachInterrupt(0, F_interrupt, RISING); // Attach the delivery interrupt so that we can wake up when mail is delivered.
  attachInterrupt(1, F_interrupt, RISING); // Attach the retrieve interrupt so that we can wake up when mail is retrieved.

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Most power savings in this sleep mode.
  sleep_enable(); // Enable sleep.

  digitalWrite(xbPower, HIGH); // Put the XBee to sleep.

  // Turn off the ADC
  keepADCSRA = ADCSRA; // Record the current state of the ADCSRA
  ADCSRA = 0; // Turn off the ADC

  // Turn off brown-out enable in software
  MCUCR = _BV (BODS) | _BV (BODSE);  // Turn on brown-out enable select.
  MCUCR = _BV (BODS); // Must be done within 4 clock cycles of above.

  sleep_mode(); // Go to sleep

  sleep_disable(); // When woken up will continue to process from this point.

  ADCSRA = keepADCSRA; // Turn on the ADC

  F_process(); // Figure out what switch woke up the Arduino and act accordingly.
}

void F_interrupt() {
}

void F_process() {
  if (digitalRead(deliverSW) == HIGH && delivery == false) { // Only run if mail has not been delivered yet. Otherwise, go back to sleep.
    digitalWrite(notifyLED1, HIGH); // Turn ON the external notification LEDs.
    digitalWrite(notifyLED2, HIGH); // Turn ON the external notification LEDs.

    digitalWrite(xbPower, LOW); // Turn ON the XBee.
    delay(xbWakeupDelay); // Give the XBee some time to settle after waking up.

    notify = 1; // Set the notify to 1 (1=delivered).

    getSensorReadings(); // Get the temp and batt voltage.

    transmitData(); // Send the notify, temp, and batt data out serial via the XBee.

    delivery = true; // Set this so we know the mail has been delivered and don't run the delivery sequence again before the retrieve door is opened.
  }

  if (digitalRead(retrieveSW) == HIGH && delivery == true) { // Only run if mail has been delivered yet. Otherwise, go back to sleep.
    digitalWrite(notifyLED1, LOW); // Turn OFF the external notification LEDs.
    digitalWrite(notifyLED2, LOW); // Turn OFF the external notification LEDs.

    // Turn ON the dome LED on until the retrieve door is closed so we can see inside the mailbox if it's dark outside.
    while (digitalRead(retrieveSW) == HIGH) {
      digitalWrite(domeLED, HIGH);
    }

    digitalWrite(domeLED, LOW); // Turn the dome LED off.

    digitalWrite(xbPower, LOW); // Wake up the XBee.
    delay(xbWakeupDelay); // Give the XBee some time to settle after waking up before doing anything with it.

    notify = 0; // Set the notify to 0 (0=delivered).

    getSensorReadings(); // Get the temp and batt voltage.

    transmitData(); // Send the notify, tem, and batt data out the serial via XBee.

    delivery = false; // Set this bak to false so we get notified when mail is delivered again.
  }
}

void getSensorReadings() {
  // Get the battery voltage.
  battPinReading = analogRead(battery);
  battPinVoltage = battPinReading * 0.00322; // 3.3V / 1024 divisions = 0.00322 volts per division.
  battVoltage = battPinVoltage * dividerRatio;

  // Get the temp.
  sensors.requestTemperatures(); // Tell the sensor to record the temp to its memory.
  tempC = sensors.getTempCByIndex(0); // Retrieve the temp from the sensor's memory and store it in a local var.
  tempF = (tempC * 9.0) / 5.0 + 32.0; // Convert to fahrenheit.
}

void transmitData() {
  // For the sake of making sure the house notification gets its ON signal we send the ON command out, wait 5.5 seconds and then send it out again until transmitCount is equal to timesToTransmit.
  for (int t = 0; t != transmitTimes; t++) {
    Serial.print("t");
    Serial.print(tempF);
    Serial.print("v");
    Serial.print(battVoltage);
    Serial.print("n");
    Serial.print(notify);
    delay(xbTransmitDelay); // Give some time for the transmission to end before we send the next one.
  }
}
