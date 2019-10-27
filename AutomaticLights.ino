/*
   Written on 13th of March, 2018
   Equipment: Arduino Uno,
              3x 5v relay switches,
              PIR sensor,
              PhotoCell sensor,
              Magnet Door switch,
              Master Button,
              Mini breadboard
   Using the Door magnet with a PIR sensor on one side of the door, I determin the direction of movement
   by checking which action happened first between the PIR and door magenet.
   I have 3 relays, one controlling the main light switch, and 2 lamps for chilled lighting. The chilled
   lighting turns on after a minute and a half of entering the room.
*/


#include <elapsedMillis.h>

//Pin values
const int LIGHT_DETECT_PIN = A0;
const int BUTTON = 12;
const int PIR_PIN = 3;
const int DOOR_PIN = 2;
const int RELAY_MAIN_PIN = 7;
const int RELAY_LAMP1_PIN = 8;
const int RELAY_LAMP2_PIN = 10;


//temp varaibles
unsigned int light_detect_raw = 0;
int button_state = 0;
int pir_raw = 0;
int door_state = 0;

//deciders
bool lights_on = false;  //True when lights are on
bool entered = true;     //True when someone enteres
bool hasLeft = true;     //True when someone leaves, if not reactivated by motion in room
bool initalLightStage = false; //True when first light is turned on

//Timelogs
#define ONE_HALF 90000  //1.5 minutes in milliseconds
#define THREE 180000  //3 minutes in milliseconds
#define NINE 540000 //9 minutes in milliseconds
#define TEN 600000  //10 minutes in milliseconds
#define ELEVEN 660000  //11 minutes in milliseconds
#define THIRTEEN 780000 //13 minutes in milliseconds
#define TOO_DARK 800 //Photosensor trigger  (900 dark, 180 bright)
elapsedMillis timeElapsed_door = 1000;
elapsedMillis timeElapsed_pir;
elapsedMillis timeElapsed_forLampSwitchOver;
elapsedMillis timeElapsed_afterLeaving = 200000;

void setup() {
  Serial.begin(9600);
  //PIR
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), motion_detected, RISING);
  //RELAY
  pinMode(RELAY_MAIN_PIN, OUTPUT);
  pinMode(RELAY_LAMP1_PIN, OUTPUT);
  pinMode(RELAY_LAMP2_PIN, OUTPUT);
  digitalWrite(RELAY_MAIN_PIN, LOW);
  digitalWrite(RELAY_LAMP1_PIN, LOW);
  digitalWrite(RELAY_LAMP2_PIN, LOW);
  //DOOR
  pinMode(DOOR_PIN, INPUT);
  //LIGHT
  pinMode(LIGHT_DETECT_PIN, INPUT);
  //BUTTON
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {  
  //printValues();
  //Check state of button
    if(button_state != digitalRead(BUTTON)){
      button_state = digitalRead(BUTTON);
       if(button_state == LOW){
        button_pressed();
      }
    }
    
  //Check if door state has changed.
  if (door_state != digitalRead(DOOR_PIN)) {
    door_state = digitalRead(DOOR_PIN);
    if (door_state == HIGH) {
      timeElapsed_door = 0;
      //Serial.println("DOOR OPEN");
    } else {
      //Serial.println("DOOR CLOSED");
    }
  }

  //Determin movement direction
  if (entered && timeElapsed_door < timeElapsed_pir) {
    //Serial.println("you left the room");
    entered = false;
    if (lights_on) {
      timeElapsed_afterLeaving = 0;
      hasLeft = true;
    }
  } else if (!entered && timeElapsed_door > timeElapsed_pir) {
    int diffBetweenSensors = timeElapsed_door - timeElapsed_pir;
    if(diffBetweenSensors < ELEVEN){ //Stops random shakes of sensor tripping sensor, or wakes in the night
      //Serial.println("you entered the room");
      entered = true;
      if (lights_on) {
        hasLeft = false;
        timeElapsed_afterLeaving = 200000;
      } else {
        if (checkIfDarkEnough()) {
          lights_on = true;
          turnOnMainLighting();
          timeElapsed_forLampSwitchOver = 0;
          initalLightStage = true;
        }
      }
    }
  }

  //Switches to chilled lighting
  if (lights_on && initalLightStage && timeElapsed_forLampSwitchOver > ONE_HALF) {
    turnOnChilledLighting();
  }

  //Switches lights off after leaving
  if (!entered && lights_on && hasLeft && timeElapsed_afterLeaving > TEN) {
    turnOffLighting(true);
    initalLightStage = true;
    hasLeft = false;
    lights_on = false;
  }
}

bool checkIfDarkEnough(){
  light_detect_raw = analogRead(LIGHT_DETECT_PIN);
  //Serial.println(light_detect_raw);
  return (light_detect_raw > TOO_DARK);
}

void turnOnMainLighting(){
     //Serial.println("Turning main lights on");
     digitalWrite(RELAY_MAIN_PIN, HIGH);
}

void turnOnChilledLighting(){
    //Serial.println("Turning chillded lights on");
    digitalWrite(RELAY_LAMP2_PIN, HIGH);
    delay(500); //for effect?
    digitalWrite(RELAY_LAMP1_PIN, HIGH);
    delay(500);
    digitalWrite(RELAY_MAIN_PIN, LOW);
}

void turnOffLighting(bool slowly){
    //Serial.println("Turning all lights off");
    digitalWrite(RELAY_MAIN_PIN, LOW);
    if(slowly) delay(500);
    digitalWrite(RELAY_LAMP1_PIN, LOW);
    if(slowly) delay(500);
    digitalWrite(RELAY_LAMP2_PIN, LOW);
}


//Is called when motion is detected
void motion_detected() {
  //Serial.println("Motion detected");
  if(timeElapsed_door < 6000){
    timeElapsed_pir = 0;
  }
  if(timeElapsed_afterLeaving < THIRTEEN && timeElapsed_afterLeaving > TEN){
    if(checkIfDarkEnough()){
      timeElapsed_pir = 0;
      hasLeft = false;
      turnOnChilledLighting();
      lights_on = true;
    }
  }
  if (lights_on) {
    timeElapsed_pir = 0;
    hasLeft = false;
  }
}

void button_pressed() {
  //Serial.println("Button has been pressed");
  if (lights_on) {
    turnOffLighting(false);
    delay(2000); //Delay to give a chance to block sensor turning lights back on.
    button_state = HIGH;
    entered = false;
    timeElapsed_afterLeaving = ELEVEN;
    timeElapsed_door = 7001;
    timeElapsed_pir = 18000;
    lights_on = false;
  } else {
    turnOnChilledLighting();
    entered = true;
    lights_on = true;
  }
  initalLightStage = false;
  hasLeft = false; //we want this false here so the lights do not reactivate through motion instantly.
  timeElapsed_afterLeaving = 200000;
}


void printValues(){
  Serial.println("timeElapsed_door : timeElapsed_pir : timeElapsed_forLampSwitchOver : timeElapsed_afterLeaving"); 
  Serial.print(timeElapsed_door);
  Serial.print(" : "); 
  Serial.print(timeElapsed_pir);
  Serial.print(" : "); 
  Serial.print(timeElapsed_forLampSwitchOver);
  Serial.print(" : "); 
  Serial.print(timeElapsed_afterLeaving);
  Serial.println();
  Serial.println("lights_on : entered : hasLeft : initialLightStage"); 
  Serial.print(lights_on);
  Serial.print(" : "); 
  Serial.print(entered);
  Serial.print(" : "); 
  Serial.print(hasLeft);
  Serial.print(" : "); 
  Serial.print(initalLightStage);
  Serial.println();
  Serial.println("button_state : light_detect_raw : door_state");
  button_state = digitalRead(BUTTON);
  light_detect_raw = analogRead(LIGHT_DETECT_PIN);
  door_state = digitalRead(DOOR_PIN);
  Serial.print(button_state);
  Serial.print(" : "); 
  Serial.print(light_detect_raw);
  Serial.print(" : "); 
  Serial.print(door_state); 
  Serial.println();
}


