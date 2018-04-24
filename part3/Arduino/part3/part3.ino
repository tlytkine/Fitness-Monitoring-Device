#include <Adafruit_Si7021.h>
#include <LiquidCrystal.h> // Library used to interact with LCD 
#include <Wire.h>



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

#define DS3231_I2C_ADDRESS 0x68
#define PROCESSING_VISUALIZER 1 
#define SERIAL_PLOTTER  2 



byte secondGet;
byte minuteGet;
byte hourGet;
byte dayOfWeekGet;
byte dayOfMonthGet;
byte monthGet;
byte yearGet;

// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return( (val/16*10) + (val%16) );
}

LiquidCrystal lcd(12,11,5,4,3,2); // Configures LCD with corresponding pins 

//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat
int realTime = 8;                  // pin that configures real time data 
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int prevBPM = 0;                  // previously stored BPM value 

String command; // String that reads in command from terminal 
String showX = "shX"; // string for showX command 
String pause = "PAU\r"; // string for pause command 
String resume_var = "RES\r"; // string for resume command 
String write_var = "WRT\r";
String env_var = "ENV\r"; // environment sensor variable 
String low = "LOW\r";
String high = "HIG\r";
String hist = "HST\r";
String dateCmd  = "DTE\r";


// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year);
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year);


boolean paused = false; // Boolean that is changed based on commands sent from terminal


static int outputType = SERIAL_PLOTTER;

Adafruit_Si7021 sensor = Adafruit_Si7021(); // declares environment sensor 
int envPin1 = 4; // A4
int envPin2 = 5; // A5 
void interruptSetup();

void setup(){

 
  pinMode(blinkPin,OUTPUT);         // pin for LED to blink to heart rate 
  pinMode(realTime,OUTPUT);          // pin that turns on and off depending on whether data is real time or not 
  Serial.begin(9600);             // baud rate of 9600 
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  lcd.begin(16,2); // configures LCD with 16 columns and 2 rows 
  // Environment sensor code 
  sensor.begin();
  bool parse = false;
  bool config = false;

  Wire.begin();


  const char *system_date = __DATE__;
  const char *system_time = __TIME__;
      


  // might need to be byte *
  byte secondSet = ((int)system_time[6]-48)*10 + (int)system_time[7]-48;
  byte minuteSet = ((int)system_time[3]-48)*10 + (int)system_time[4]-48;
  byte hourSet = ((int)system_time[0]-48)*10 + (int)system_time[1]-48;
  byte dayOfWeekSet = 2; // 1 Sunday, 7 Saturday 
  byte dayOfMonthSet = system_time[5];
  byte monthSet = 04;
  byte yearSet = 18;
  /*lcd.print(system_time);
  lcd.print(" ");
  lcd.print(hourSet);
  lcd.print(minuteSet);
  lcd.print(secondSet);
  */
  setDS3231time(secondSet,minuteSet,hourSet,dayOfWeekSet,dayOfMonthSet,monthSet,yearSet);

  
}
  

void loop(){


      
      //delay(500); // every second
    if(paused==false){ // if no terminal commands received then proceed 
      // Serial.write(BPM); // writes BPM stored to serial port 
      // Serial.write("\n");
      // Serial.flush();
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
    // lcd.clear();
    // lcd.setCursor(0,0);
    // lcd.write("got command");

    
    // String comparison is used to perform the corresponding functions 
    // of the commands received from the terminal 
    // Serial.flush() and a delay is performed to assure functionality of 
    // all of the commands 

    // If the resume command is received from the terminal,
    // the LCD screen will continue to display realtime data
    // and the real time LED will stay on similarly 
    // to what is shown above, resume is used after showX or pause 
    /* Part 3: resume: Show the real-time heart rate on the display device. 
     *  This should be the default mode of the system. Indicate this somehow in your circuit 
     *  (using an LED for example).
     * 
     */
    if(command.equals(resume_var)){
        paused = false;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.write("BPM: ");
        lcd.setCursor(0,1);
        lcd.print(BPM);
        Serial.write("\n");
        //delay(100);
    }
    // showX command prints X to the screen instead of the current BPM
    // and turns the real time LED off 
    /* Part 3: showX: Set the output device to show X as the current heart rate instead of the current 
     *  real-time value. In addition, print the value to the console.
     */
    else if(showX.equals(command.substring(0,3))){
      paused = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("BPM: ");
      lcd.setCursor(0,1);
      lcd.print(command.substring(3));
     // Serial.write(BPM);
      Serial.write("\n");                                                                                                                                                                                                                                                                                                                                                                                                   Serial.write("\n");
     // Serial.flush();
      //delay(100);
    }
    // pause will keep the current BPM displayed on the screen 
    // and turns the real time LED off 
    /* Part 3: pause: Pause the output and keep the display device showing the current reading. 
     *  Indicate this somehow in your circuit (using an LED for example).
     * 
     */
    else if(command.equals(pause)){
      paused = true;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("BPM: ");
      lcd.setCursor(0,1);
      lcd.print(BPM);
      // Serial.write(BPM);
      Serial.write("\n");
      // Serial.flush();
      //delay(100);
    }
    else if(command.equals(write_var)){
      readDS3231time(&secondGet,&minuteGet,&hourGet,&dayOfWeekGet,&dayOfMonthGet,&monthGet,&yearGet);
      //lcd.clear();
      //lcd.setCursor(0,0);
      //lcd.write("BPM: ");
      //lcd.setCursor(0,1);
      //lcd.print(BPM);
      Serial.write(BPM);
      Serial.write(hourGet);
      Serial.write(minuteGet);
      Serial.write(secondGet);
      Serial.write("\n");
      //Serial.flush();
      //delay(100);
    }
    // env_var = "ENV"\r"
    // env: Query the value of the environment sensor from the Arduino and print 
    // it to the console 
    else if(command.equals(env_var)){

        char temperature = sensor.readTemperature();
        char humidity = sensor.readHumidity();
        Serial.write(temperature);
        Serial.write(humidity);
        Serial.write("\n");
    }
    else if(command.equals(low)){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("Warning: ");
      lcd.setCursor(0,1);
      lcd.print("BPM Low");
     // Serial.write(BPM);
      Serial.write("\n");                                                                                                                                                                                                                                                                                                                                                                                                   Serial.write("\n");
     // Serial.flush();
      //delay(100);
    }
    else if(command.equals(high)){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.write("Warning: ");
      lcd.setCursor(0,1);
      lcd.print("BPM High");
     // Serial.write(BPM);
      Serial.write("\n");                                                                                                                                                                                                                                                                                                                                                                                                   Serial.write("\n");
     // Serial.flush();
      //delay(100);
    }
    else if(command.equals(hist)){
      readDS3231time(&secondGet,&minuteGet,&hourGet,&dayOfWeekGet,&dayOfMonthGet,&monthGet,&yearGet);
      Serial.write(BPM);
      Serial.write(hourGet);
      Serial.write(minuteGet);
      Serial.write(secondGet);
      Serial.write("\n");
    }
    else if(command.equals(dateCmd)){
      readDS3231time(&secondGet,&minuteGet,&hourGet,&dayOfWeekGet,&dayOfMonthGet,&monthGet,&yearGet);
      Serial.write(dayOfMonthGet);
      Serial.write(monthGet);
      Serial.write(yearGet);
      Serial.write("\n");
      
    }
  }
  
  if(QS == true){     // A Heartbeat Was Found        
        prevBPM = BPM;         // used to save prevBPM for debugging purposes       
  }                  
  delay(IBI);                           

}

void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year)
{
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set next input to start at the seconds register
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth, byte *month, byte *year)
{
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}




