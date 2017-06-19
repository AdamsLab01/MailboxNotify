/**********************************************************************************************************************
  Mailbox Notifier - Repeater Side

  This sketch was written to work as a Snail mail delivery notification system. It uses two LEDs mounted to the mailbox
  as local (to the mailbox) delivery notification. It also sends out delivery/retrieve data via an XBee modem (serial).
  When the receiving XBee (set in a remote location, like your house) receives the data it turns a delivery indicator
  light on or off and records and displays the delivery/retrieve data.

  This code is for the repeater that I needed to put in place to get the signal from the mailbox to inside our house.
  The signal was weak to begin with (the distance between the mailbox and reciveris ~120'), but after haivng new windows
  put in (the oxides in the windows atinuate the signal) it was impossible to get a signal directly from the mailbox.
  Since I was adding it, I put a DS18B20 temp sensor in the repeater as well. The repeater sits outside under the evave 
  of the house near the front window. 

  For more information see - http://adambyers.com/2013/11/mailbox-notifier/ or ping adam@adambyers.com

**********************************************************************************************************************/


#include "Wire.h"
#include "OneWire.h"
#include "DallasTemperature.h"

int transmitTimes = 4; // How many times we transmit the mailbox data.
int xbTransmitDelay = 6000; // Number of milliseconds we wait between XBee transmissions, so the transmissions are not stepping on each other.
int notify = 3; // The notify state, 1=delivered 0=retrieved (set to 3 here so we don't trip it on bootup)
float battVoltage = 0; // Calculated battery voltage
float tempMailboxF = 0; // Mailbox temp in fahrenheit
float tempRepeaterF = 0; // Repeater temp in fahrenheit
float tempC = 0; // Repeater temp reading in celsius.

// DS18B20 setup
#define tempSensor 2 // PIN the DS18B20 is connected to.
OneWire oneWire(tempSensor);
DallasTemperature sensors(&oneWire); // Pass the OneWire reference to the DS18B20.

void setup() {
  Serial.begin(9600);

  sensors.begin(); // Start the DallasTemperature library.
}

void loop() {

  reciveData(); // Receive data from serial via XBee.

  getSensorReadings(); // Get the temp sensor reading.

  delay(5000); // For sanity, probably not nessesary.

  transmitData(); // Send the notify, temp, and batt data out the serial via XBee.

}

void reciveData() {
  /*
    Data from the mailbox is sent our via serial/XBee in the format t75.00v3.45n1 where t=temperature v=voltage n=notify
    The following reads the incoming serial data, parses and records it based on the indicator (t,v,n) preceding each indicator.
  */

  while (Serial.available()) { // Determine if there is anything to read from serial.
    if (Serial.find("t")) // Find the 't' char that denotes where the Temp reading begins.
      tempMailboxF = Serial.parseFloat(); // Parse the float # after the 't' char to get the temp reading.

    if (Serial.find("v"))  // Find the 'v' char that denotes where the Voltage reading begins.
      battVoltage = Serial.parseFloat(); // Parse the float # after the 'v' char to get the voltage reading.

    if (Serial.find("n"))  // Find the 'n' char that denotes where the Notify state begins.
      notify = Serial.parseInt(); // Parse the int # after the 'n' char to get the Notify state.
  }
}

void getSensorReadings() {
  // Get the temp.
  sensors.requestTemperatures(); // Tell the sensor to record the temp to its memory.
  tempC = sensors.getTempCByIndex(0); // Retrieve the temp from the sensor's memory and store it in a local var.
  tempRepeaterF = (tempC * 9.0) / 5.0 + 32.0; // Convert to fahrenheit.
}

void transmitData() {
  // To make sure the house gets the data from the mailbox we send it the number of times defined by transmitTimes, wait 6 seconds and then send it out again until transmitCount is equal to transmitTimes.
  for (int t = 0; t != transmitTimes; t++) {
    Serial.print("m");
    Serial.print(tempMailboxF);
    Serial.print("r");
    Serial.print(tempRepeaterF);
    Serial.print("v");
    Serial.print(battVoltage);
    Serial.print("n");
    Serial.print(notify);
    delay(xbTransmitDelay); // Give some time for the transmission to end before we send the next one.
  }
}

