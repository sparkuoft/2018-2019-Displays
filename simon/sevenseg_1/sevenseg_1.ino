#include <EEPROM.h>
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_LEDBackpack.h"

//Buttons
unsigned long buttonPin[6] = {A5,10,11,12,13,8};    
unsigned long ledPin[6] = {2,5,7,6,4,3};      
unsigned long button0[7];
unsigned long button1[7];
unsigned long button2[7];
unsigned long button3[7];
unsigned long button4[7];
unsigned long button5[7]; //StartButton
unsigned long * Buttons[6] = {button0,button1,button2,button3,button4,button5};
unsigned long debounceDelay = 50;    // increase if needed

//Constants
int PRESET = 5;
const int MAXGAME = 125;
int address = 0;  //EEPROM address (0,1,2,3 4 bytes in total)
//Buzzer 
const int BUZZER_PIN = 9; //Digital Pin
const int BUZZER_FEQ = 1000; //HZ (cycles per second)
const int BUZZER_DUR = 500; // length of the notes
const int c = 261;
const int e = 329;
const int g = 391;
const int cH = 523;
const int eH = 659;
const int feq[5] = {c, e, g, cH, eH};

//Global Variables
bool STARTGAME = false;
bool FAILED = false;
bool RESET = false;
bool SEQED = false;
bool WAIT = false;
bool LIGHT = false;
//Score
unsigned long HighScore = 0;
unsigned long currScore = 0;
//Sequence
int sequence[MAXGAME];
int currLen = 0;
int currNote = 0;
int currPressed = 0;
int currGame = 0;
unsigned long last_active;
unsigned long sequence_delay;
//LED Fade
int led_fadeAmount=5; 
unsigned long led_delay;
// 7 Seg Display
Adafruit_7segment matrix = Adafruit_7segment();
Adafruit_7segment matrix2 = Adafruit_7segment();

void setup() {
  matrix.begin(0x70);   // RED - Your Score
  matrix2.begin(0x71);  // GREEN - (A0) High Score
  Serial.begin(9600);
  for (int i=0; i <= 5; i++){
    initializeButton( (Buttons[i]), buttonPin[i], ledPin[i]);
  }
  last_active = millis();
  sequence_delay = millis();
  FAILED = false;
  RESET = false;
  WAIT = false;
  LIGHT = false;
  currNote = 0;
  currPressed = 0;
  currScore = 0;
  currGame = 0;
  currLen = 0;
  testeeprom(); //Read high score from eeprom
  printScore();
  buzzerScale();
}



void loop() {
  STARTGAME = RESET; 
  RESET = false;
  FAILED = false;
  //Reset Start Button
  (Buttons[5])[3] = 0; 
  led_fadeAmount=5;
  (Buttons[5])[6] = 0;
  
  //REFRESH FIX-TODO
  if (PRESET >0){
    PRESET--;
    STARTGAME = true;
  }

  //IDLE
  while(!STARTGAME){
    ledFade(Buttons[5]);
    buttonToggled_PWM(Buttons[5]);
    STARTGAME = ((Buttons[5])[3]); //pressed
  }
  
  //Reset Buttons
  for (int i=0; i <= 4; i++){
      (Buttons[i])[6] = 0;
      digitalWrite((Buttons[i])[5], (Buttons[i])[6]);    
  }
  //Reset Game
  currLen = 0;
  for (int i=0; i < MAXGAME; i++){
    sequence[i]=6;  //6 is invalid
  }

  //Begin Game
  start();
}

void start(){
  FAILED = false;
  RESET = false;
  WAIT = false;
  LIGHT = false;
  currNote = 0;
  currPressed = 0;
  currScore = 0;
  currGame = 0;
  currLen = 0;
  currScore = 0;
  printScore();
  (Buttons[5])[3] = 0; 
  //Refresh all button press
  for (int i=0; i <= 4; i++){
    (Buttons[i])[3] = 0;
    (Buttons[i])[6] = 0;
  }
  while(!FAILED && !RESET && (currGame < MAXGAME)){
  //  extendOne();
  //  lightSequence()
    buttonToggled_PWM(Buttons[5]);
    RESET = ((Buttons[5])[3]); //pressed
  
    //currLen = 3;
    if (currLen==0){
      extendOne();
    }
    
    
    lightSequence();
    if ((millis()-last_active) >= 10000){
      endGame();
    }
    waitSequence();
    
  }
}

void extendOne(){
  int temp;
  temp = random(0,6);
  if (temp >= 5){
    temp = 3;
  }
  sequence[currLen] = (int)temp;
  currLen++;
  LIGHT = true;
  WAIT = false;
  last_active = millis();
}

void endGame(){
  //blink score
  FAILED = true;
  currNote = 0;
  for (int i=0; i <= 4; i++){
    (Buttons[i])[6] = 1;
    digitalWrite((Buttons[i])[5], (Buttons[i])[6]);
  }
  buzzerScale();
  delay(1000);
  for (int i=0; i <= 4; i++){
    (Buttons[i])[6] = 0;
    digitalWrite((Buttons[i])[5], (Buttons[i])[6]);
  }
}

void endSequence(){
  extendOne();
  currGame++;
  currNote = 0;
  delay(500);
}

void waitSequence(){
  int pressed = 0;
  int note2;
  currPressed = 0;
  SEQED = false;
  while(!FAILED && !RESET && WAIT && !LIGHT){
    if ((millis() -  last_active) >= 10000){
      endGame();
    }
    buttonToggled_PWM(Buttons[5]);
    RESET = ((Buttons[5])[3]); //pressed
    for (int i=0; i <= 4; i++){
      (Buttons[i])[3]=0;
      buttonToggled(Buttons[i]);
      pressed = 0;
      pressed = (Buttons[i])[3];
      if (pressed){
        tone(BUZZER_PIN, feq[i], 100);
        if (currPressed < currLen){
          currPressed++;
          note2 = sequence[currPressed-1];
          if(note2 !=6){
            if (i==note2){
              currScore++;
              printScore();
              Serial.print(222);
              if (currPressed >= currLen){
                Serial.print(444);
                //Sequence Complete
                endSequence();
                WAIT = false;
                SEQED = true;
                return;
              }
            }
            else{
              Serial.print(333);
              //Wrong Note Pressed
              endGame();
              SEQED = true;
              return;
            }
          }
        }
        last_active = millis();
      }
    }
  }
  
}

void lightSequence(){
  int note;
  //sequence_delay = millis();
  if ((currLen > 0) && (currLen <=MAXGAME) && LIGHT){
    if (millis() -  sequence_delay >= 700){
      if (currNote <= currLen){
        currNote++;
        note = sequence[currNote-1];
        if(note!=6){
          (Buttons[note])[6] = 1;
          tone(BUZZER_PIN, feq[note], BUZZER_DUR);
          sequence_delay = millis(); 
          last_active = millis();
        }
      }
      if (currNote > currLen){
        currNote++;
        note = sequence[currNote-1];
        if(note!=6){
          (Buttons[note])[6] = 0;
          sequence_delay = millis(); 
          last_active = millis();
        }
      }
    }
    else if (millis() -  sequence_delay >= 500){
      note = sequence[currNote-1];
      if(note!=6){
        (Buttons[note])[6] = 0;
      }
    }
    else{
      note = sequence[currNote-1];
      if(note!=6){
        (Buttons[note])[6] = 1;
      }
    }
  digitalWrite((Buttons[note])[5], (Buttons[note])[6]);
  }
  else if (currLen == 0 && LIGHT){
    if (currNote < currLen){
        currNote++;
        note = sequence[currNote-1];
        if(note!=6){
          (Buttons[note])[6] = 1;
          sequence_delay = millis(); 
          last_active = millis();
          digitalWrite((Buttons[note])[5], (Buttons[note])[6]);
          tone(BUZZER_PIN, feq[note], BUZZER_DUR);
        }
      }
  }
  //else{ TODO FIX - ending 'wining' sequence for completing max games
  // When all sequence displayed
  if (currNote > currLen){
    WAIT = true;
    LIGHT = false;
  }
}

void buttonToggled_PWM(unsigned long *buttonObject){
  // led lights up when pressed
  int reading = digitalRead(buttonObject[0]);
  // If the switch changed, due to noise or pressing, reset the debouncing timer
  if (reading != buttonObject[1]) {
    buttonObject[4] = millis();
  }
  if ((millis() - buttonObject[4]) > debounceDelay) {
    if (reading != buttonObject[2]) { 
      //If state has changed
      buttonObject[2] = reading;
      if (buttonObject[2] == LOW){
        buttonObject[6] = 255;
      }
      if (buttonObject[2] == HIGH) {
        Serial.print('released');
        buttonObject[6] = 0;
      }
      buttonObject[3] = buttonObject[2]==HIGH ? 1:0;
    }
  }
  analogWrite(buttonObject[5], buttonObject[6]);
  buttonObject[1] = reading;
}
void buttonToggled(unsigned long *buttonObject){
  // led lights up when pressed
  int reading = digitalRead(buttonObject[0]);
  // If the switch changed, due to noise or pressing, reset the debouncing timer
  if (reading != buttonObject[1]) {
    buttonObject[4] = millis();
  }
  if ((millis() - buttonObject[4]) > debounceDelay) {
    if (reading != buttonObject[2]) { 
      //If state has changed
      buttonObject[2] = reading;
      if (buttonObject[2] == LOW){
        //Serial.print('pressed');
        buttonObject[6] = 1;
      }
      if (buttonObject[2] == HIGH) {
        Serial.print('released');
        buttonObject[6] = 0;
      }
      buttonObject[3] = buttonObject[2]==HIGH ? 1:0;
    }
  }
  digitalWrite(buttonObject[5], buttonObject[6]);
  buttonObject[1] = reading;
}
void initializeButton(unsigned long *buttonObject, unsigned long pinID, unsigned long ledPin){
  buttonObject[0] = pinID;
  buttonObject[1] = 0;  // lastButtonState
  buttonObject[2] = 0;  // buttonState
  buttonObject[3] = 0;  // buttonPressed
  buttonObject[4] = millis();   //lastDebounceTime
  buttonObject[5] = ledPin;
  buttonObject[6] = 1;  //ledState
  //Serial.print(pinID);
  pinMode((int)pinID, INPUT);
  digitalWrite((int)pinID, HIGH);  //PullUp
  pinMode((int)ledPin, OUTPUT);
  digitalWrite((int)ledPin, (int)buttonObject[6]);
}

void ledFade(unsigned long *buttonObject)  { 
  if (millis() - led_delay >= 30){
    led_delay = millis();
    analogWrite(buttonObject[5], buttonObject[6]);    
    buttonObject[6] = buttonObject[6] + led_fadeAmount;
    // reverse the direction of the fading at the ends of the fade: 
    if (buttonObject[6] == 0 || buttonObject[6] == 255) {
      led_fadeAmount = -led_fadeAmount ; 
    }     
  }                            
}

void buzzerScale() {
  for (int i=4; i>=0; i--){
    tone(BUZZER_PIN, feq[i], BUZZER_DUR);
    delay(200);
  }
}

void printScore() {
  currScore = min(9999,currScore);  //cast both scores to 9999 here
  matrix.print(currScore, DEC);
  matrix.writeDisplay();
  if (currScore > HighScore){
    HighScore = currScore;
    printHighScore();
  }
}

void printHighScore(){
  matrix2.print(HighScore, DEC);
  matrix2.writeDisplay();
  unsigned long value1 = HighScore;
  byte four1 = (value1 & 0xFF);
  byte three1 = ((value1 >> 8) & 0xFF);
  byte two1 = ((value1 >> 16) & 0xFF);
  byte one1 = ((value1 >> 24) & 0xFF);
  EEPROM.write(address, four1);
  EEPROM.write(address + 1, three1);
  EEPROM.write(address + 2, two1);
  EEPROM.write(address + 3, one1);
}

void testeeprom(){
//  CLEAR EEPROM
//  long value1 = 0000;
//  byte four1 = (value1 & 0xFF);
//  byte three1 = ((value1 >> 8) & 0xFF);
//  byte two1 = ((value1 >> 16) & 0xFF);
//  byte one1 = ((value1 >> 24) & 0xFF);
//  EEPROM.write(address, four1);
//  EEPROM.write(address + 1, three1);
//  EEPROM.write(address + 2, two1);
//  EEPROM.write(address + 3, one1);
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  HighScore = ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
  Serial.print(HighScore);
  printHighScore();
}
