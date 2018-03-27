
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
#include <LiquidCrystal.h> // Library used to interact with LCD 
#define PROCESSING_VISUALIZER 1 
#define SERIAL_PLOTTER  2 


LiquidCrystal lcd(12,11,5,4,3,2); // Configures LCD with corresponding pins 

//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int realTime = 8;                  // pin that configures real time data 
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int prevBPM = 0;                  // previously stored BPM value 

String command; // String that reads in command from terminal 
String showX = "shX\r"; // string for showX command 
String pause = "PAU\r"; // string for pause command 
String resume_var = "RES\r"; // string for resume command 

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.



boolean paused = false; // Boolean that is changed based on commands sent from terminal


static int outputType = SERIAL_PLOTTER;


void setup(){
  pinMode(blinkPin,OUTPUT);         // pin for LED to blink to heart rate 
  pinMode(realTime,OUTPUT);          // pin that turns on and off depending on whether data is real time or not 
  Serial.begin(9600);             // baud rate of 9600 
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  lcd.begin(16,2); // configures LCD with 16 columns and 2 rows 

}



void loop(){

    if(paused==false){ // if no terminal commands received then proceed 
      Serial.write(BPM); // writes BPM stored to serial port 
      lcd.clear(); // clears LCD 
      lcd.setCursor(0,0); // sets the cursor to the top left 
      lcd.print("BPM: "); // prints "BPM: " in the first row 
      lcd.setCursor(0,1); // sets the cursor to the bottom left 
      lcd.print(BPM); // prints actual real time BPM value in second row 
      digitalWrite(realTime, HIGH); // turns LED on since data is real time 
    }
    else{
      digitalWrite(realTime, LOW); // turns LED off since data is not real time 
    }
    


  
  if(Serial.available()){ // checks if serial port is available 
    command = Serial.readString(); // reads in command from terminal to a string 
    
    // String comparison is used to perform the corresponding functions 
    // of the commands received from the terminal 
    // Serial.flush() and a delay is performed to assure functionality of 
    // all of the commands 

    // If the resume command is received from the terminal,
    // the LCD screen will continue to display realtime data
    // and the real time LED will stay on similarly 
    // to what is shown above, resume is used after showX or pause 
    if(command.equals(resume_var)){
        paused = false;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.write("BPM: ");
        lcd.setCursor(0,1);
        lcd.print(BPM);
        delay(100);
    }
    // showX command prints X to the screen instead of the current BPM
    // and turns the real time LED off 
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
    // pause will keep the current BPM displayed on the screen 
    // and turns the real time LED off 
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
        prevBPM = BPM;         // used to save prevBPM for debugging purposes       
  }                  
  delay(IBI);                           

}

