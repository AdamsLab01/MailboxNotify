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
int timesToTransmit = 10;

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
  attachInterrupt(0, F_Interrupt, RISING);
  attachInterrupt(1, F_Interrupt, RISING);
  
  digitalWrite(XBPower, HIGH);
  digitalWrite(DomeLED, LOW);
  
  delay(100); 
  
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Most power savings in this mode
  
  sleep_enable(); // Enable sleep
  
  sleep_mode(); // Go to sleep
  
  // When woken up will continue to process from this point
  sleep_disable(); 
 
  state = S_process; // Figure out what switch woke up the Arduino and act accordingly
} 

void F_Interrupt() {

}

void F_process() { 
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
    digitalWrite(NotifyLED1, HIGH);
    digitalWrite(NotifyLED2, HIGH);
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(10000); // Give the radio some time to settle after waking up
    
    while(transmitCount != timesToTransmit){  
      Serial.println("1"); // Send a character out the serial to turn on the house notification
      delay(1000);
      transmitCount ++;
    }
    
    transmitCount = 0;
    delivery = true;
    state = S_sleep;
  }
  
  else if (delivery == true) {
    state = S_sleep;
  }
}

void F_retrieve() {
  if (delivery == true) {
    digitalWrite(NotifyLED1, LOW);
    digitalWrite(NotifyLED2, LOW);
   
    while (digitalRead(RetrieveSW) == HIGH) {
      digitalWrite(DomeLED, HIGH); // Stay here to keep the mailbox dome LED on while the retrieve door is opened
    } 
    
    digitalWrite(DomeLED, LOW);
    digitalWrite(XBPower, LOW); // Turn the radio on
    delay(10000); // Give the radio some time to settle after waking up
    
    while(transmitCount != timesToTransmit){  
      Serial.println("0"); // Send a character out the serial to turn on the house notification
      delay(1000);
      transmitCount ++;
    }
    
    transmitCount = 0;
    delivery = false;
    state = S_sleep;
  }

  else if (delivery == false) {
    state = S_sleep;
  }
}
