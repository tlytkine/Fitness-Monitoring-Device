
/*  Pulse Sensor Amped 1.5    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ----------------------
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat    PIN 8
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
 ----------------------       ----------------------  ----------------------
*/
#include <LiquidCrystal.h>
#define PROCESSING_VISUALIZER 1
#define SERIAL_PLOTTER  2


LiquidCrystal lcd(12,11,5,4,3,2);

//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int realTime = 8;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int prevBPM = 0;                  // previously stored BPM value 

String command;
String showX = "shX\r";
String pause = "PAU\r";
String resume_var = "RES\r";

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.



boolean paused = false; // Boolean that is changed based on terminal output 


static int outputType = SERIAL_PLOTTER;


void setup(){
  pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  pinMode(realTime,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(9600);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  lcd.begin(16,2);

}



void loop(){

    if(paused==false){
      Serial.write(BPM);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("BPM: ");
      lcd.setCursor(0,1);
      lcd.print(BPM);
      digitalWrite(realTime, HIGH);
    }
    else{
      digitalWrite(realTime, LOW);
    }
    


  
  if(Serial.available()){
    command = Serial.readString();
    

    if(command.equals(resume_var)){
        paused = false;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.write("BPM: ");
        lcd.setCursor(0,1);
        lcd.print(BPM);
        delay(100);
    }
    else if(command.equals(showX)){
      paused = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("BPM: ");
      lcd.setCursor(0,1);
      lcd.print("X");
      Serial.print(BPM);
      Serial.flush();
      delay(100);
    }
    else if(command.equals(pause)){
      paused = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("BPM: ");
      lcd.setCursor(0,1);
      lcd.print(BPM);
      Serial.print(BPM);
      Serial.flush();
      delay(100);
    }
  }

  
  if (QS == true){     // A Heartbeat Was Found        
        prevBPM = BPM;               
  }                  
  delay(IBI);                           

}

