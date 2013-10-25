// PIN Names
int LightSW = 2;

void setup() {
  pinMode(LightSW, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  char LightState = Serial.read();
  
  if(LightState == '1'){  	 
    digitalWrite(LightSW, HIGH);
  }
  
  else if(LightState == '0'){
    digitalWrite(LightSW, LOW);
  }
}
