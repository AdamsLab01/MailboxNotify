/**********************************************************************************************************************
Mailbox Notifier - House Side

This sketch was written to work as a Snail mail box delivery notification system. It uses to LEDs mounted to the mailbox
as local (to the mailbox) delivery notifications. It also sends out a serial character via a connected XBee modem. When 
the receiving XBee (set in a remote location like in your house) receives the character it either turns an indicator
light on or off.

For more information see - http://adambyers.com/2013/11/mailbox-notifier/ or ping adam@adambyers.com

All code (except external libraries and third party code) is published under the MIT License.

**********************************************************************************************************************/

// PIN Names
int LightSW = 2;

void setup() {
  pinMode(LightSW, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  while(Serial.available()){  // Determine if there is anything to read from serial
    
    char LightState = Serial.read();  // If there is something to read from serial, read it

      if(LightState == '0'){  	 
        digitalWrite(2, LOW);
      }
	
      else if(LightState == '1'){
        digitalWrite(2, HIGH);
      }
    }
}