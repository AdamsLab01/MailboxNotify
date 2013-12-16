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

// States
#define S_sleep 1
#define S_process 2
#define S_deliver 3
#define S_retrieve 4

// Default start up state
int state = S_sleep;

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
int timesToTransmit = 20; // Number of times we will send out the ON or OFF signal out for the house notification. This can be reduced if the distance/interferace of the enviornmen is low
int XBeeWakeupDelay = 15000; // Number of milliseconds we wait after waking the XBee before using it
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
  switch(state) {
    
    case S_sleep:
      F_SleepyTime();
    break;
  
    case S_process:
      F_process();
    break;
  
    case S_deliver:
      F_deliver();
    break;
    
    case S_retrieve:
      F_retrieve();
    break;
  }
}

void F_SleepyTime() {
  attachInterrupt(0, F_Interrupt, RISING); // Attach the interrupt so that we can wake up when mail is delivered
  attachInterrupt(1, F_Interrupt, RISING); // Attach the interrupt so that we can wake up when mail is retrieved
  
  digitalWrite(XBPower, HIGH); // Put the XBee to sleep
  
  delay(100); // Slight delay for sanity
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Most power savings in this mode
  
  sleep_enable(); // Enable sleep
  
  sleep_mode(); // Go to sleep
  
  sleep_disable(); // When woken up will continue to process from this point
 
  state = S_process; // Figure out what switch woke up the Arduino and act accordingly
} 

void F_Interrupt() {
// Empty funciton for interrupt
}

void F_process() { 
  // Functionally unnecessary but for good mesure we detach the interrupts while we are not using them
  detachInterrupt(0);
  detachInterrupt(1);
  
  if (digitalRead(DeliverSW) == HIGH){
    state = S_deliver;
  }
  
  if (digitalRead(RetrieveSW) == HIGH) {
    state = S_retrieve;
  } 
}
 
void F_deliver() {  
  if (delivery == false) {
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(XBeeWakeupDelay); // Give the radio some time to settle after waking up
    
    // For the sake of making sure the house notification gets its ON signal we send the ON command out, wait 1.5 seconds and then send it out again until transmitCount is equal to timesToTransmit
    while(transmitCount != timesToTransmit){  
      Serial.println("1"); // Send a character out the serial to turn on the house notification
      delay(XBeeTransmitDelay); // Give some time for the transmition to end before we send the next one
      transmitCount ++;
    }
    
    // Turn the external notification LEDs on
    digitalWrite(NotifyLED1, HIGH);
    digitalWrite(NotifyLED2, HIGH);
    
    transmitCount = 0; // Reset
    delivery = true;
    state = S_sleep;
  }
  
  // To make sure we conserve power we just go right back to sleep if the delevery door is opened after we have already detcted delivery
  else if (delivery == true) {
    state = S_sleep;
  }
}

void F_retrieve() {
  if (delivery == true) {
    // Turn off the external notification LEDs
    digitalWrite(NotifyLED1, LOW);
    digitalWrite(NotifyLED2, LOW);
   
    // Turn on the dome LED on untill the retrive door is closed so we can see inside the mailbox if it's dark outside
    while (digitalRead(RetrieveSW) == HIGH) {
      digitalWrite(DomeLED, HIGH);
    } 
    
    digitalWrite(DomeLED, LOW); // Turn the dome LED off
    
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(XBeeWakeupDelay); // Give the radio some time to settle after waking up before doing anything with it
    
    // For the sake of making sure the house notification gets its OFF signal we send the OFF command out, wait 1.5 seconds and then send it out again until transmitCount is equal to timesToTransmit
    while(transmitCount != timesToTransmit){  
      Serial.println("0"); // Send a character out the serial to turn off the house notification
      delay(XBeeTransmitDelay);
      transmitCount ++;
    }
    
    transmitCount = 0; // Reset
    delivery = false;
    state = S_sleep;
  }

  // To make sure we conserve power we just go right back to sleep if the the retrive door is opened when a delivery has not been detected
  else if (delivery == false) {
    state = S_sleep;
  }
}
