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