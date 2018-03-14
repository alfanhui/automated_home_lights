/*
 * Written on 13th of March, 2018
 * Equipment: Arduino Uno, 
 *            3x 5v relay switches, 
 *            PIR sensor, 
 *            PhotoCell sensor, 
 *            Magnet Door switch, 
 *            Master Button, 
 *            Mini breadboard
 * Using the Door magnet with a PIR sensor on side of the door, I determin the direction of movement
 * by checking which sequence happened first between the PIR and door magenet was actived. 
 * I have 3 
 */


#include <elapsedMillis.h>

//Pin values
const int LIGHT_DETECT_PIN = A0;
const int BUTTON = 1;
const int PIR_PIN = 2;
const int DOOR_PIN = 3;
const int RELAY_MAIN_PIN = 8;
const int RELAY_LAMP1_PIN = 9;
const int RELAY_LAMP2_PIN = 10;


//temp varaibles
unsigned int light_detect_raw = 0;
int button_state = 0;
int pir_raw = 0;
int door_state = 0;

//deciders
bool lights_on = false;  //True when lights are on
bool entered = true;     //True when someone enteres
bool hasLeft = true;     //True when someone leaves
bool initalLightStage = false; //True when first light is turned on
bool hasLeftOnce = false; //True when someone leaves, untrue if motion is detected before lights turn off

//Timelogs
#define ONE_HALF 90000  //1.5 minutes in milliseconds
#define THREE 180000  //3 minutes in milliseconds
#define TEN 600000  //10 minutes in milliseconds
elapsedMillis timeElapsed_door = 1000;
elapsedMillis timeElapsed_pir;
elapsedMillis timeElapsed_forLampSwitchOver;
elapsedMillis timeElapsed_afterLeaving = 200000;

 void setup(){
   Serial.begin(9600);
   //PIR
   attachInterrupt(digitalPinToInterrupt(PIR_PIN), motion_detected, RISING);
   attachInterrupt(digitalPinToInterrupt(BUTTON), button_pressed, RISING);
   //RELAY
   pinMode(RELAY_MAIN_PIN, OUTPUT);
   pinMode(RELAY_LAMP1_PIN, OUTPUT);
   pinMode(RELAY_LAMP2_PIN, OUTPUT);
   digitalWrite(RELAY_MAIN_PIN, LOW);
   digitalWrite(RELAY_LAMP1_PIN, LOW);
   digitalWrite(RELAY_LAMP2_PIN, LOW);
   //DOOR
   pinMode(DOOR_PIN, INPUT_PULLUP);
   //LIGHT
   pinMode(LIGHT_DETECT_PIN, INPUT);
   //BUTTON
   pinMode(BUTTON, INPUT);
  }

 void loop(){
  //Check if door state has changed.
  if(door_state != digitalRead(DOOR_PIN)){
    door_state = digitalRead(DOOR_PIN); 
     if(door_state == HIGH){
        timeElapsed_door=0;
        Serial.println("DOOR OPEN"); 
      }else{
        Serial.println("DOOR CLOSED");
     }
  }

  //Determin movement direction
  if(entered && timeElapsed_door < timeElapsed_pir){
    Serial.println("you left the room");
    entered = false;
    if(lights_on){
      timeElapsed_afterLeaving = 0;
      hasLeftOnce = true;
    }
  }else if(!entered && timeElapsed_door > timeElapsed_pir){
    Serial.println("you entered the room");
    entered = true;
    if(lights_on){
      if(hasLeft){
        hasLeft = false;
      }
    }else{
      light_detect_raw = analogRead(LIGHT_DETECT_PIN);  //900 dark, 180 bright
      if(light_detect_raw > 800){
        Serial.println("turning on lights");
        lights_on = true;
        digitalWrite(RELAY_MAIN_PIN, HIGH);
        timeElapsed_forLampSwitchOver = 0;
        initalLightStage = true;
      }
    }
  }

  //Switches to chilled lighting
  if(lights_on && initalLightStage && timeElapsed_forLampSwitchOver > ONE_HALF){
    initalLightStage = false;
    digitalWrite(RELAY_LAMP2_PIN, HIGH);
    delay(500); //for effect?
    digitalWrite(RELAY_LAMP1_PIN, HIGH);
    delay(500);
    digitalWrite(RELAY_MAIN_PIN, LOW);
  }

  //Switches lights off after leaving
  if(!entered && lights_on && hasLeftOnce && timeElapsed_afterLeaving > TEN){
    Serial.println("Turning all lights off");
    digitalWrite(RELAY_MAIN_PIN, LOW);
    delay(500); 
    digitalWrite(RELAY_LAMP1_PIN, LOW);
    delay(500); 
    digitalWrite(RELAY_LAMP2_PIN, LOW);   
    initalLightStage = true;
    hasLeftOnce = false; 
    lights_on = false;
  }
 }

 //Is called when motion is detected
 void motion_detected(){
  Serial.println("Motion detected");
  timeElapsed_pir = 0;
  if(lights_on && timeElapsed_afterLeaving < TEN){
    hasLeftOnce = false;
  }
 }

 void button_pressed(){
  Serial.println("Button has been pressed");
  if(lights_on){
    Serial.println("Turning all lights off");
    digitalWrite(RELAY_MAIN_PIN, LOW);
    delay(500); 
    digitalWrite(RELAY_LAMP1_PIN, LOW);
    delay(500); 
    digitalWrite(RELAY_LAMP2_PIN, LOW); 
  }else{
    digitalWrite(RELAY_LAMP1_PIN, HIGH);
    digitalWrite(RELAY_LAMP2_PIN, HIGH);
    lights_on = true;
    initalLightStage = false;
  }
 }

