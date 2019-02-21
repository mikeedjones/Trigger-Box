//TODO: Add serial comms. so that it tells the PC what it's doing and accepts all commands from PC.

#include <Wire.h>
#include <LiquidCrystal_I2C.h> // https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include "SimpleTimerMAH.h"
#include<EnableInterrupt.h>

//FUNCTION PROTOTYPES
void lcdInit(); //Populate display with initial values
void encRotEvent(); //Count encoder events and update the 
void encButtonEvent();
void fireButtonEvent();
void countLaserPulses();

//GLOBAL VARIABLES
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address. Set the pins on the I2C chip used for LCD connections:  addr, en,rw,rs,d4,d5,d6,d7,bl,blpol

volatile unsigned int n=1; //let through every n/divisor pulse
volatile unsigned pulseCount=1;
volatile bool fire=false;
volatile bool fired=false;
volatile bool firing=false;
volatile bool singleShot=true;
volatile bool changeMade=true;

//setup pins
const byte laserClk=2;
const byte encPinA=3;
const byte encPinB=4;
const byte encButton=5;
const byte fireButton=6;
const byte gate=7;

//setup timers
const int encDebounceTime=20;
const int minLoopUpdateTime=100;
SimpleTimer debounce(encDebounceTime);
SimpleTimer loopTimer(100);
SimpleTimer idleTimeOut(60000);//turn off backlight if device is not touched for 60 seconds and the device is paused

void setup() {
  Serial.begin(115200);
  Serial.println("Connected");

  lcdInit();
  
  pinMode(encPinA, INPUT_PULLUP);  // Configure the pin as an input, and turn on the pullup resistor.
  pinMode(encPinB, INPUT_PULLUP);
  pinMode(laserClk, INPUT);
  pinMode(encButton, INPUT_PULLUP);
  pinMode(fireButton, INPUT_PULLUP);
  pinMode(gate, OUTPUT);
  enableInterrupt(encPinA, encRotEvent, FALLING);
  enableInterrupt(laserClk, countLaserPulses, FALLING);
  enableInterrupt(encButton, encButtonEvent, FALLING);
  enableInterrupt(fireButton, fireButtonEvent, FALLING);
}

void loop() {

  /*if(idleTimeOut.timedOut()==true && firing==false){
    lcd.noBacklight();
  } 
  else if(fire=true){
    lcd.backlight();
  }*/
  if(idleTimeOut.timedOut(false)==true){
    lcd.noBacklight();
  } 
  else{
    lcd.backlight();
  }
  
  if(changeMade){//only update the display if there has been a state change. Likewise for sending serial.
    idleTimeOut.reset();
    changeMade=false;
    if(singleShot){
      lcd.setCursor(6,0);
      lcd.print("SingleShot");
      lcd.setCursor(0,1);
      lcd.print("                ");
    }else{
      lcd.setCursor(6,0);
      lcd.print("Pulsed     ");
      lcd.setCursor(0,1);
      lcd.print("N:              "); 
      lcd.setCursor(3,1);
      lcd.print(n);
      if(firing==true){
        lcd.setCursor(10,1);
        lcd.print("Firing");
      }else{
        lcd.setCursor(10,1);
        lcd.print("Paused");
      }
    }
  }
  //display update
  if(loopTimer.timedOut()){
//    //lcd.setCursor(9,1);
//    //lcd.print(n);
//    //Serial.println(pulseCount);
//    Serial.println(singleShot);
//  Serial.print("n: ");
//  Serial.print(n);
//  Serial.print(", p: ");
//  Serial.println(pulseCount);
//  Serial.print("timeout: ");
//  Serial.print(idleTimeOut.elapsed());
//  Serial.print("state: ");
//  Serial.println(idleTimeOut.timedOut(false));
  }
}

void lcdInit(){
  //LCD setup
  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  //LCD initial write
  lcd.setCursor(0,0); 
  lcd.print("Mode: SingleShot");
}

void encRotEvent(){
  bool A=digitalRead(encPinA);
  bool B=digitalRead(encPinB);
  if(singleShot==false){//only update n if in correct mode because it's not displayed on LCD otherwise so change will be unknown
    if(debounce.timedOut()){
      firing=false;//switch off output if you make a change to prevent multiple pulses getting through. Push button to restart
      changeMade=true; //this will update the display even if no valid event is detected but shouldn't matter
      if(A==true && B==false){n--;}
      else if(A==false && B==true){n++;}
      else if(A==true && B==true){n++;}
      else if(A==false && B==false){n--;}
      if(n<1){n=1;}
    }
  }
}

void encButtonEvent(){
  changeMade=true;
  firing=false;
  singleShot = !singleShot;
}

void fireButtonEvent(){
  Serial.write("B");
  if(singleShot){
    fire=true;
    idleTimeOut.reset();
  }else{
    changeMade=true;
    firing = !firing;
  }
}

void countLaserPulses(){

    if(singleShot==true){//single shot button/PC mode
      if(fire==true){
        digitalWrite(gate,HIGH);
        fired=true;
        fire=false;
      } else if(fired==true){
        fired=false;
        digitalWrite(gate,LOW);
      }
    } else{//fire every n events provided fireButton has started the pulsing
      if(firing==true){
        pulseCount++;
        if(n!=1){
          if(pulseCount == n){
            //if(fired=false){
            fired=true;
            digitalWrite(gate,HIGH);
          }
          else if(pulseCount > n){
            pulseCount=1;
            fired=false;
            digitalWrite(gate,LOW);
          }
        }else{
          fired=true;
          digitalWrite(gate,HIGH);
        }
      }else if(fired=true){
        digitalWrite(gate,LOW);//makes sure gate goes low if firing mode exited and it wasn't auto cancelled
      }
    }
  
}


