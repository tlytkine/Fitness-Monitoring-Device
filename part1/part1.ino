/*
 You will also write a C program to run on a UNIX host that communicates with the Arduino. The program
will expose a command prompt that supports the following commands:
• show X: Set the output device to show X as the current heart rate instead of the current real-time value.
This will be useful while debugging.
• pause: Pause the output and keep the display device showing the current reading
• resume: Show the real-time heart rate on the display device. This should be the default mode of the
system
• exit: Exits the host program
 */




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

}
