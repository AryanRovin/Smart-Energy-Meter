// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"
//#include <SPI.h>
//#include <SD.h>
//#include<SoftwareSerial.h>

//SoftwareSerial gsm(10,11);// 2 is rx,3 is tx
//File myFile;

// include the library code:
#include <LiquidCrystal.h> //library for LCD

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

 int case1 = 0;
 int bill = 0;
  
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9, 8, 7, 6, 5, 4); // RS, E, D4, D5, D6, D7
 
#define volt A2

int AC_volt = 0;
float voltage = 0 ;
float sensorValue = 0;
float average = 0;
int power = 0;
long prev_time = 0;
float units = 0;
int final_energy = 0;
float energy = 0;

int hours,mins,secs = 0;
//Measuring Current Using ACS712

int ct = A3;
float nVPP;   // Voltage measured across resistor
float nCurrThruResistorPP; // Peak Current Measured Through Resistor
float nCurrThruResistorRMS; // RMS current through Resistor
float nCurrentThruWire;     // Actual RMS current in Wire
float ecurrent;     // Actual RMS current in Wire

int pri_relay0 = A0;
int sec_relay0 = A1;
int sec_relay1 = 12;
int vib = 13;

void setup()
{
 //baud rate
 Serial.begin(9600);//baud rate at which arduino communicates with Laptop/PC
 //gsm.begin(9600);
 
 Serial.println("AT+CNMI=2,2,0,0,0"); // AT Command to recieve a live SMS
 
 pinMode(volt,INPUT);  
 pinMode(ct, INPUT);
 pinMode(vib,INPUT); 
 
 pinMode(pri_relay0,OUTPUT); 
 pinMode(sec_relay0,OUTPUT); 
 pinMode(sec_relay1,OUTPUT); 

 digitalWrite(pri_relay0,HIGH);
 digitalWrite(sec_relay0,HIGH);
 digitalWrite(sec_relay1,HIGH);
 
 
 // set up the LCD's number of columns and rows:
 lcd.begin(20, 4); //LCD order
 // Print a message to the LCD.
 lcd.setCursor(0,0);//Setting cursor on LCD
 lcd.print("     GSM Based ");//Prints on the LCD
 lcd.setCursor(0,1);
 lcd.print("       Meter  ");

 //if (!SD.begin(10)) 
 {
 //   Serial.println("SD card initialization failed!");
    //while (1);
  }
  //Serial.println("initialization done.");
   
  Serial.begin(9600);

  if (! rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) 
  {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
 delay(100);//time delay for 3 sec
 lcd.clear();//clearing the LCD display

}
 
void loop() //method to run the source code repeatedly
{

 // check tempering
 if(digitalRead(vib) == HIGH)
 {
  // TEMPER ALERT
  Serial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(10);  // Delay of 1000 milli seconds or 1 second
  Serial.println("AT+CMGS=\"+92323999999999\"\r"); // Replace x with mobile number
  delay(10);
  Serial.println("Temper_Alert");// The SMS text you want to send
  delay(10);
  Serial.println(units);// The SMS text you want to send
  Serial.println((char)26);// ASCII code of CTRL+Z
  delay(10); 
 }
 
 if(Serial.available() > 0)
 {
  char text = Serial.read();
  Serial.println(text);
  Serial.println("Msg recieved,");
  delay(500);
  
  if(text == '*')
  {
    Serial.println("Sending units update");
    SendMessage();
  }
  else
  {
    Serial.println("Invalid Text Recieved");
  }
 }
 else
 {
 get_time ();
 
 for (int i=0; i < 3; i++) 
 {
 sensorValue = analogRead(volt);
 // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 250V):
 voltage = (sensorValue/1024) * 260;       
 average = average + voltage ;
 }
    
  AC_volt = (average/3);      // read the input on analog pin 0:
  average = 0;
      
  //CALCULATE CURRENT
  nVPP = getVPP();
   
   /*
   Use Ohms law to calculate current across resistor
   and express in mA 
   */
   
   nCurrThruResistorPP = (nVPP/780.0) * 1500.0;
   
   /* 
   Use Formula for SINE wave to convert
   to RMS 
   */
   
   nCurrThruResistorRMS = nCurrThruResistorPP * 0.707;
   
   /* 
   Current Transformer Ratio is 1000:1...
   
   Therefore current through 200 ohm resistor
   is multiplied by 1000 to get input current
   */
   
   nCurrentThruWire = nCurrThruResistorRMS ;
   
   if(nCurrentThruWire < 0.1)
   {
    ecurrent = 0 ;
   }
   else
   {
   ecurrent = nCurrentThruWire - 0.05 ;
   }
   
   
   
 //  Serial.println();

   // calculate power
   power = ecurrent * AC_volt ;

  // print out the value you read:
  lcd.setCursor(0,1) ;
  lcd.print("V:");
  lcd.setCursor(3,1) ;
  lcd.print(AC_volt);
  
 lcd.setCursor(8,1);
 lcd.print("I:");
 lcd.setCursor(12,1);
 lcd.print(ecurrent);
 //lcd.setCursor(12,2);
 //lcd.print("A"); //unit for the current to be measured
 //delay(200);
 //lcd.clear();
   
 lcd.setCursor(0,2);
 lcd.print("P:");
 lcd.setCursor(3,2);
 lcd.print(power);
 //lcd.setCursor(8,3);
 //lcd.print("Watts"); //unit for the current to be measured

 //energy = ( power * (1000)/(3600 *200)) ;  // kwatt - hr
 energy = power * 0.4; ;
 final_energy = abs(energy + final_energy);
 //prev_time = millis ();
 
 // print out the value you read:
  lcd.setCursor(8,2) ;
  lcd.print("Wh");
  lcd.setCursor(12,2) ;
  lcd.print(final_energy);
 // lcd.setCursor(6,1) ;
 // lcd.print("V");
 // delay(200);
 // lcd.clear();
  
   units = abs(final_energy/1000)  ;
   //Serial.print("Units:");
   //Serial.print(units);
   //Serial.println(" ");
   
   // print out the value you read:
  lcd.setCursor(0,3) ;
  lcd.print("Units");
  lcd.setCursor(6,3) ;
  lcd.print(units);
  //delay(00);

   bill = units * 12;
   lcd.setCursor(11,3) ;
   lcd.print("Bill:");
   lcd.setCursor(16,3) ;
   lcd.print(bill);
  
 energy = 0;
 delay(200); //delay of 2.5 sec
 lcd.clear();
}
}

void get_time ()
{
    DateTime now = rtc.now();

    hours = now.hour();
    mins = now.minute();
    secs = now.second();

    if(hours >= 1 && hours <=24)
    {
      if(power > 10)
      {
        if(case1 == 0)
        {
       //primary load turned off
       digitalWrite(pri_relay0,LOW);
       Serial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(10);  // Delay of 1000 milli seconds or 1 second
  Serial.println("AT+CMGS=\"+90000000000\"\r"); // Replace x with mobile number
  delay(10);
  Serial.println("Overload in Peak hours");// The SMS text you want to send
  delay(10);
  Serial.println(" Primary load turned off");// The SMS text you want to send
  Serial.println((char)26);// ASCII code of CTRL+Z
  delay(300);
        case1 = 1;
        }    }
      else
      {
      // digitalWrite(pri_relay0,HIGH);
       }
     }
       else
       {
        case1 = 0; 
        digitalWrite(pri_relay0,HIGH);
       }
 
    //Serial.print(hours, DEC);
    //Serial.print(':');
    //Serial.print(mins, DEC);
    //Serial.print(':');
    //Serial.print(secs, DEC);
    //Serial.println();
    //lcd.clear();
    lcd.setCursor(0,0);lcd.print(hours);lcd.print(":");
    lcd.setCursor(3,0);lcd.print(mins);lcd.print(":");
    lcd.setCursor(7,0);lcd.print(secs);
    delay(50);
    //lcd.clear();
    
}

float getVPP()
{
  float result;
  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  uint32_t start_time = millis();
  
   while((millis()-start_time) < 100) //sample for 1 Sec
   {
       readValue = analogRead(ct);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
   }

   // Convert the digital data to a voltage
   result = (maxValue * 5.0)/1024.0;
     // Serial.println(result);  
   return result;
 }

 void SendMessage()
{

  Serial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(10);  // Delay of 1000 milli seconds or 1 second
  Serial.println("AT+CMGS=\"+98888888888\"\r"); // Replace x with mobile number
  delay(10);
  Serial.println("Volts:");// The SMS text you want to send
  delay(10);
  Serial.println(AC_volt);// The SMS text you want to send

  Serial.println("Current:");// The SMS text you want to send
  delay(10);
  Serial.println(ecurrent);// The SMS text you want to send

  Serial.println("Power:");// The SMS text you want to send
  delay(10);
  Serial.println(power);// The SMS text you want to send
  
  Serial.println("Units:");// The SMS text you want to send
  delay(10);
  Serial.println(units);// The SMS text you want to send
  delay(10);
  Serial.println("Bill:");// The SMS text you want to send
  delay(10);
  Serial.println(bill);// The SMS text you want to send
  Serial.println((char)26);// ASCII code of CTRL+Z
  delay(10);

}
