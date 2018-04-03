void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  int input;
  int num;
  int buff[64];
  int j = -1;
  if(Serial.available()>0){
    input = Serial.read();

    if(input==',')
    {
      /* num = calc(); */
      j = -1;
      Serial.println(num);
    }
    else 
    {
      j++;
      buff[j]=input;
    }
  }

}
