/**********************************************************************************************************************
Mailbox Notifier - House Side

This sketch was written to work as a Snail mail delivery notification system. It uses two LEDs mounted to the mailbox
as local (to the mailbox) delivery notification. It also sends out delivery/retrieve data via an XBee modem (serial). 
When the receiving XBee (set in a remote location, like your house) receives the data it turns a delivery indicator
light on or off and records and displays the delivery/retrieve data.

For more information see - http://adambyers.com/2013/11/mailbox-notifier/ or ping adam@adambyers.com

**********************************************************************************************************************/

//Libraries
#include "Wire.h"
#include "RTClib.h"
#include "LiquidCrystal.h"

RTC_DS1307 rtc; // Define the RTC

// PIN Names
int lightSW = 2;
int hourPlusSW = 9;
int hourMinusSW = 10;

// LCD PINs
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);

// Vars
float temp = 0;
float voltage = 0;
bool delivered = false;
bool retrieve = false;
int notify = 3; // This will come in via serial as 0 or 1. Set to 3 here to keep it from triggering.
// Current data/time vars
int cYear = 0;
int cMonth = 0;
int cDay = 0;
int cHour = 0;
int cMin = 0;
int cSecond = 0;
// Delivery vars
float dTemp = 0;
float dVoltage = 0;
int dYear = 0;
int dMonth = 0;
int dDay = 0;
int dHour = 0;
int dMin = 0;
int dSecond = 0;
// Retrieve vars
float rTemp = 0;
float rVoltage = 0;
int rYear = 0;
int rMonth = 0;
int rDay = 0;
int rHour = 0;
int rMin = 0;
int rSecond = 0;
// LCD update vars
long prevLCDMillis = 0;
long lcdInt = 2000; // Time to display each "screen."
int numScreen = 9; // How many "screens" we have, less 1.  
int screenNum = 0;  
bool screenChanged = true;
// Button 1 debounce vars
int buttonState1;
int lastButtonState1 = HIGH;
long lastDebounceTime1 = 0;
long debounceDelay = 5;
// Button 2 debounce vars
int buttonState2;
int lastButtonState2 = HIGH;
long lastDebounceTime2 = 0;

// LCD update states 
#define C_date 0
#define C_time 1
#define D_date 2
#define D_time 3
#define D_temp 4
#define D_voltage 5
#define R_date 6
#define R_time 7
#define R_temp 8
#define R_voltage 9

void setup() {
  pinMode(lightSW, OUTPUT);
  pinMode(hourPlusSW, INPUT);
  pinMode(hourMinusSW, INPUT);
  
  digitalWrite(hourPlusSW, HIGH); // Enable internal resistor for switch.
  digitalWrite(hourMinusSW, HIGH); // Enable internal resistor for switch.
  
  Serial.begin(9600);
  
  Wire.begin();
  
  rtc.begin();
  
  lcd.begin(8, 2);
  
/*
To set the date/time:

1. Uncomment the line: 

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

2. Upload the sketch

3. Comment out the line:

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
4. Upload the sketch.

The RTC will be set to the system date/time recorded when compiled. After, you MUST comment out the 'rtc.adjust...' line 
and upload the sketch again. Form this point forward the RTC will keep track of the date/time itself. Failure to comment 
out the 'rtc.adjust...' line and re-upload will cause the date/time to be reset to the compiled time if the Arduino 
is reset. 
*/

   rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Uncomment to set RTC to system time.
}

void loop() { 
  DateTime now = rtc.now(); // Get the current time & date (for display).
  
  // Added so we can add or subtract an hour from the RTC if need be (stupid DST). Would have been cleaner to write this as a function rather than repeting it for both buttons.
  int reading1 = digitalRead(hourPlusSW) == LOW;
  
  if (reading1 != lastButtonState1) {
    lastDebounceTime1 = millis();
  }
  
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading1 != buttonState1) {
      buttonState1 = reading1;
      if (buttonState1 == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() + 1, now.minute()));
      }
    }
  }
  
  lastButtonState1 = reading1;
  // Done debouncing and depositing.
  
  int reading2 = digitalRead(hourMinusSW) == LOW;
  
  if (reading2 != lastButtonState2) {
    lastDebounceTime2 = millis();
  }
  
  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading2 != buttonState2) {
      buttonState2 = reading2;
      if (buttonState2 == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), now.hour() - 1, now.minute()));
      }
    }
  }
  
  lastButtonState2 = reading2;

  // Record the current time and date for display.
  cYear = (now.year()) - 2000; // Subtract 2000 so we get a 2-digit date and can fit it on the LCD.
  cMonth = (now.month());
  cDay = (now.day());
  cHour = (now.hour());
  cMin = (now.minute()); 
  cSecond = (now.second());
  
  /*
  Data from the mailbox is sent to the house via serial in the format t75.00v3.45n1 where t=temperature v=voltage n=notify
  The following reads the incoming serial data and parses it based on the indicator (t,v,n) preceding each value. 
  */
  
  while(Serial.available()) {  // Determine if there is anything to read from serial.
    if (Serial.find("t")) // Find the 't' char that denotes where the Temp reading begins.
      temp = Serial.parseFloat(); // Parse the float # after the 't' char to get the temp reading.
    
    if (Serial.find("v"))  // Find the 'v' char that denotes where the Voltage reading begins.
      voltage = Serial.parseFloat(); // Parse the float # after the 'v' char to get the voltage reading.
      
    if (Serial.find("n"))  // Find the 'n' char that denotes where the Notify state begins.
      notify = Serial.parseInt(); // Parse the int # after the 'n' char to get the Notify state. 
  }

  /*
  Data is from the mailbox is transmitted 4 times to insure that the house received the data (since it's transmitting blindly, 
  i.e. there is no handshake or received confirmation). We only need one good transmission though and can assume that once we 
  know what the notify setting is we've received a good transmission and can stop listening. We do this by setting 
  'delivered == true' and 'retrieve == true' after we've run each sequence.   
  */

  if (notify == 1 && delivered == false) { // If mail has been delivered and we have not run this sequence already.
    digitalWrite(lightSW, HIGH); // Turn on the notification light.
    
    DateTime now = rtc.now(); // Get the current time & date.
    
    // Record the delivery time and date
    dYear = (now.year()) - 2000; // Subtract 2000 so we get a 2-digit date and can fit it on the LCD.
    dMonth = (now.month());
    dDay = (now.day());
    dHour = (now.hour());
    dMin = (now.minute()); 
    dSecond = (now.second());
    
    // Record the delivery temp and voltage.
    dTemp = temp;
    dVoltage = voltage;
    
    // We assume that since mail is being delivered that it has not yet been retrieved and reset the retrieve vars.
    rYear = 0;
    rMonth = 0;
    rDay = 0;
    rHour = 0;
    rMin = 0; 
    rSecond = 0;
    rTemp = 0;
    rVoltage = 0;
    
    retrieve = false; // Reset
    delivered = true; // Don't run this again till it's false.
  }
  
  if (notify == 0 && retrieve == false) { // If mail has been retrieved and we have not run this sequence already.
    digitalWrite(lightSW, LOW); // Turn off the notification light.
    
    DateTime now = rtc.now(); // Get the current time & date.

    // Record the retrieve time and date.
    rYear = (now.year()) - 2000; // Subtract 2000 so we get a 2-digit date and can fit it on the LCD.
    rMonth = (now.month());
    rDay = (now.day());
    rHour = (now.hour());
    rMin = (now.minute()); 
    rSecond = (now.second());
    
    // Record the retrieve temp and voltage.
    rTemp = temp;
    rVoltage = voltage; 
  
    retrieve = true;
    delivered = false;  
  }

  // Screen update - based off code from 'robtillaart' in the Arduino forums.
  unsigned long currentLCDMillis = millis();
  
  if (currentLCDMillis - prevLCDMillis > lcdInt) {
    prevLCDMillis = currentLCDMillis; 
    screenNum++;
    if (screenNum > numScreen) screenNum = 0;  // If all defined screens have been shown, start over.
      screenChanged = true;
  }

  if (screenChanged == true) {
    screenChanged = false; // Reset for next round.
    switch(screenNum)    
    {
      case C_date: 
        showCdate(); 
      break;
      case C_time: 
        showCtime(); 
      break;
      case D_date: 
        showDdate(); 
      break;
      case D_time: 
        showDtime(); 
      break;
      case D_temp: 
        showDtemp();
      break;
      case D_voltage:
        showDvoltage();
      break;
      case R_date: 
        showRdate(); 
      break;
      case R_time: 
        showRtime(); 
      break;
      case R_temp: 
        showRtemp();
      break;
      case R_voltage:
        showRvoltage();
      break;
    }
  }
}

// Functions
void showCdate() {
  lcd.clear();
  lcd.setCursor(0, 0);           
  lcd.print("Cur Date");
  lcd.setCursor(0, 1);          
  lcd.print(cMonth);
  lcd.print("/");
  lcd.print(cDay);
  lcd.print("/");
  lcd.print(cYear);
}

void showCtime() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cur Time");
  lcd.setCursor(0, 1);
    
  if (cHour < 10) {
    lcd.print("0");
  }
    
  lcd.print(cHour);
  lcd.print(":");
 
  if (cMin < 10) {
    lcd.print("0");
  }
 
  lcd.print(cMin);
  lcd.print(":");
    
  if (cSecond < 10) {
    lcd.print("0");
  }
 
  lcd.print(cSecond);
}

void showDdate() {
  lcd.clear();
  lcd.setCursor(0, 0);           
  lcd.print("Del Date");
  lcd.setCursor(0, 1);          
  lcd.print(dMonth);
  lcd.print("/");
  lcd.print(dDay);
  lcd.print("/");
  lcd.print(dYear);
}

void showDtime() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Del Time");
  lcd.setCursor(0, 1);
    
  if (dHour < 10) {
    lcd.print("0");
  }
    
  lcd.print(dHour);
  lcd.print(":");
 
  if (dMin < 10) {
    lcd.print("0");
  }
 
  lcd.print(dMin);
  lcd.print(":");
    
  if (dSecond < 10) {
    lcd.print("0");
  }
 
  lcd.print(dSecond);
}

void showDtemp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Del Temp");
  lcd.setCursor(0, 1);
  lcd.print(dTemp);
  lcd.print((char)223);
}

void showDvoltage() {
  lcd.clear();
  lcd.setCursor(0, 0);           
  lcd.print("Del Volt");
  lcd.setCursor(0, 1);          
  lcd.print(dVoltage);
  lcd.print("v");
}

void showRdate() {
  lcd.clear();
  lcd.setCursor(0, 0);           
  lcd.print("Rev Date");
  lcd.setCursor(0, 1);          
  lcd.print(rMonth);
  lcd.print("/");
  lcd.print(rDay);
  lcd.print("/");
  lcd.print(rYear);
}

void showRtime() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rev Time");
  lcd.setCursor(0, 1);
  
  if (rHour < 10) {
    lcd.print("0");
  }
    
  lcd.print(rHour);
  lcd.print(":");
 
  if (rMin < 10) {
    lcd.print("0");
  }
 
  lcd.print(rMin);
  lcd.print(":");
    
  if (rSecond < 10) {
    lcd.print("0");
  }
 
  lcd.print(rSecond);
}

void showRtemp() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rev Temp");
  lcd.setCursor(0, 1);
  lcd.print(rTemp);
  lcd.print((char)223);
}

void showRvoltage() {
  lcd.clear();
  lcd.setCursor(0, 0);           
  lcd.print("Rev Volt");
  lcd.setCursor(0, 1);          
  lcd.print(rVoltage);
  lcd.print("v");
}
