/**********************************************************************************************************************
Mailbox Notifier - Mailbox Side

This sketch was written to work as a Snail mail box delivery notification system. It uses to LEDs mounted to the mailbox
as local (to the mailbox) delivery notifications. It also sends out a serial character via a connected XBee modem. When 
the receiving XBee (set in a remote location like in your house) receives the character it either turns an indicator
light on or off.

For more information see - http://adambyers.com/2013/11/mailbox-notifier/ or ping adam@adambyers.com

All code (except external libraries and third party code) is published under the MIT License.

**********************************************************************************************************************/

#include <avr/sleep.h>
#include <avr/interrupt.h>

// PIN Names
int DeliverSW = 2;
int RetrieveSW = 3;
int XBPower = 4;
int NotifyLED1 = 5;
int NotifyLED2 = 6;
int DomeLED = 7;

// Vars
bool delivery = false;
int transmitCount = 0;
int timesToTransmit = 10; // Number of times we will send out the ON or OFF signal out for the house notification. This can be reduced if the distance/interferace of the enviornmen is low
int XBeeWakeupDelay = 20000; // Number of milliseconds we wait after waking the XBee before using it
int XBeeTransmitDelay = 1500; // Number of milliseconds we wait between transmitions

void setup() { 
  // Set PINs as inputs or outputs
  pinMode(DeliverSW, INPUT);
  pinMode(RetrieveSW, INPUT);
  pinMode(XBPower, OUTPUT);
  pinMode(NotifyLED1, OUTPUT);
  pinMode(NotifyLED2, OUTPUT);
  pinMode(DomeLED, OUTPUT);
  
  // Enable internal resistors by setting the PINs as HIGH. This is so we don't have to use pesky external resistors
  digitalWrite(DeliverSW, HIGH);
  digitalWrite(RetrieveSW, HIGH);
  
  // Start your serial engines!
  Serial.begin(9600);  
}

void loop() { 
  digitalWrite(XBPower, HIGH); // Put the XBee to sleep
  delay(1000); // wait a second and then go to sleep
  F_sleep();
}

void F_sleep(void) {
  attachInterrupt(0, F_Interrupt, RISING); // Attach the interrupt so that we can wake up when mail is delivered
  attachInterrupt(1, F_Interrupt, RISING); // Attach the interrupt so that we can wake up when mail is retrieved
  
  delay(100); // Slight delay for sanity
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Most power savings in this mode
  sleep_mode(); // Go to sleep
  sleep_disable(); // When woken up will continue to process from this point
  
  F_process(); // Figure out what switch woke up the Arduino and act accordingly
}
  
void F_Interrupt() {
}

void F_process() { 
  if (digitalRead(DeliverSW) == HIGH && delivery == false) {
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(XBeeWakeupDelay); // Give the radio some time to settle after waking up
    
    // For the sake of making sure the house notification gets its ON signal we send the ON command out, wait 1.5 seconds and then send it out again until transmitCount is equal to timesToTransmit
    for (int t=0; t != timesToTransmit; t++){
      Serial.println("1"); // Send a character out the serial to turn on the house notification
      delay(XBeeTransmitDelay); // Give some time for the transmition to end before we send the next one
    }
    
    digitalWrite(NotifyLED1, HIGH); // Turn the external notification LEDs on
    digitalWrite(NotifyLED2, HIGH); // Turn the external notification LEDs on
    
    delivery = true;
}
  
  if (digitalRead(RetrieveSW) == HIGH && delivery == true) {
    digitalWrite(NotifyLED1, LOW); // Turn off the external notification LEDs
    digitalWrite(NotifyLED2, LOW); // Turn off the external notification LEDs
   
    // Turn on the dome LED on until the retrive door is closed so we can see inside the mailbox if it's dark outside
    while (digitalRead(RetrieveSW) == HIGH) {
      digitalWrite(DomeLED, HIGH);
    } 
    
    digitalWrite(DomeLED, LOW); // Turn the dome LED off
    
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(XBeeWakeupDelay); // Give the radio some time to settle after waking up before doing anything with it
    
    // For the sake of making sure the house notification gets its OFF signal we send the OFF command out, wait 1.5 seconds and then send it out again until transmitCount is equal to timesToTransmit
    for (int t=0; t != timesToTransmit; t++){
      Serial.println("0"); // Send a character out the serial to turn on the house notification
      delay(XBeeTransmitDelay); // Give some time for the transmition to end before we send the next one
    }
    
    delivery = false;
  }
}
