/* This is the code for a mad scientist themed "chemical mixing" puzzle box. Participants
should have a separate clue book with puzzles leading them to the correct answers to 
input into this "mixing station." The various mixing elements are real chemicals that
mix together, but the amounts and situation is obviously made up. If incorrect answers
are entered 3 times, the system enters a "shutdown" mode that can only be bypassed by
hitting the "reset" button.

This was made using an Arduino Nano.

The correct solution steps are:

1- Hg, S, Low Heat
2- Cl, S, No Heat
3- Hg, Cl, High Heat
4- Hg, Cl, S, No Heat
5- Hg, High Heat
6- S, Low Heat */


#include "FastLED.h"                    //FastLED library for LEDs
#include "DFRobotDFPlayerMini.h"        //Mini MP3 Player library for MP3 Player
#include "SoftwareSerial.h"             //Used to communicate with MP3 Player

#define HotPot A0                       //Potentiometer on pin A0
#define HgSwitch A1                      //Slide Switch #1 on pin D2
#define ClSwitch A2                      //Slide Switch #2 on pin D3
#define SSwitch A3                       //Slide Switch #3 on pin D4
#define MixButton 11                     //Push Button #1 on pin D5
#define RstButton 13                     //Push Button #2 on pin D6

#define BusyPin 8                       //MP3 Player "Busy" Pin on D7
#define Lock 9                         //Mini Solenoid on pin A1

#define HgLED 2                        //First set of LEDs on pin D10
#define ClLED 10                        //Second set of LEDs on pin D11
#define SLED 3                         //Third set of LEDs on pin D12
#define HotLED 5                       //Fourth set of LEDs on pin D13
#define MixLED 4                       //Fifth set of LEDs on pin A2
#define ProgLED 12                      //Sixth set of LEDs on pin A3

#define WarnLED1 A5                     //First warning LED
#define WarnLED2 A4                     //Second warning LED

#define HgLEDNum 2                      //Number of LEDs in strip
#define ClLEDNum 2
#define SLEDNum 3
#define HotLEDNum 6
#define MixLEDNum 3
#define ProgLEDNum 6

SoftwareSerial mySoftwareSerial (7, 6); //Rx, Tx. Initialize Rx/Tx communication with MP3 Player
DFRobotDFPlayerMini Speaker;            //Initialize MP3 Player and name it. Here called "Speaker"

CRGB HgLEDStrip[HgLEDNum];              //Create each LED strip
CRGB ClLEDStrip[ClLEDNum];
CRGB SLEDStrip[SLEDNum];
CRGB HotLEDStrip[HotLEDNum];
CRGB MixLEDStrip[MixLEDNum];
CRGB ProgLEDStrip[ProgLEDNum];

int Step = 0;                           //Variable to store step of puzzle completion
int Warning = 0;                        //Variable to store step of incorrect guesses
int HotPotNum = 0;                      //Variable to store potentiometer value
int HotPotPos = 0;                      //Variable to store potentiometer position
int HotPotChng = 0;                     //Variable to store previous potentiometer position
int HgState = 0;                        //Variables to store reading of buttons/switches
int ClState = 0;
int SState = 0;
int MixState = 0;
int RstState = 0;

bool HgON = false;                      //Stores value of switches on or off
bool ClON = false;
bool SON = false;
bool ShutdownSequence = false;          //Is system in shutdown yes/no
bool MixLEDChange = false;              //Have the mixing container contents been changed

void setup() {

  mySoftwareSerial.begin(9600);         //Initialze MP3 communication at 9600 baud
  Serial.begin(115200);                 //Initialize Serial Monitor for troubleshooting on 115200 baud

  pinMode(Lock, OUTPUT);                //Sets A1 pin as digital output
  pinMode(HgSwitch, INPUT_PULLUP);      //Sets slide switches as input_pullup
  pinMode(ClSwitch, INPUT_PULLUP);
  pinMode(SSwitch, INPUT_PULLUP);
  pinMode(MixButton, INPUT);            //Sets pushbuttons as input
  pinMode(RstButton, INPUT);
  pinMode(WarnLED1, OUTPUT);            //simple LEDs as output
  pinMode(WarnLED2, OUTPUT);

  if (!Speaker.begin(mySoftwareSerial)) {       //Make sure MP3 Player is responding
    Serial.println("DF Player not responding!");
    while (true);
  }

  Speaker.volume(30);                   //Set the volume, value from 0 to 30

  FastLED.addLeds<NEOPIXEL, HgLED>(HgLEDStrip, HgLEDNum);      //Initialize LED strips for FastLED
  FastLED.addLeds<NEOPIXEL, ClLED>(ClLEDStrip, ClLEDNum);
  FastLED.addLeds<NEOPIXEL, SLED>(SLEDStrip, SLEDNum);
  FastLED.addLeds<NEOPIXEL, HotLED>(HotLEDStrip, HotLEDNum);
  FastLED.addLeds<NEOPIXEL, MixLED>(MixLEDStrip, MixLEDNum);
  FastLED.addLeds<NEOPIXEL, ProgLED>(ProgLEDStrip, ProgLEDNum);

  fill_solid(HgLEDStrip, HgLEDNum, CRGB::Black);         //Begin with all LEDs off
  fill_solid(ClLEDStrip, ClLEDNum, CRGB::Black);
  fill_solid(SLEDStrip, SLEDNum, CRGB::Black);
  fill_solid(HotLEDStrip, HotLEDNum, CRGB::Black);
  fill_solid(MixLEDStrip, MixLEDNum, CRGB::Black);
  fill_solid(ProgLEDStrip, ProgLEDNum, CRGB::Black);
  FastLED.show();

  Speaker.play(7);              //Play startup sound

  delay(8000);                  //delay long enough to play entire sound

}

void loop() {

  HgState = digitalRead(HgSwitch);            //Read position of switches
  ClState = digitalRead(ClSwitch);
  SState = digitalRead(SSwitch);
  MixState = digitalRead(MixButton);
  RstState = digitalRead(RstButton);
  HotPotRead();

  if (RstState == HIGH) {               //Check for reset button push
    reset();                            //This MUST be above shutdownsequence to work
  }

  if (ShutdownSequence == true) {       //Check if in shutdown state; if so, skip loop and return to beginning
    return;
  }

  if (Step == 6) {                      //If at the end, don't allow any more inputs (except reset button)
    return;
  }

  if ((MixLEDChange == true) &&((HgON == true) || (ClON == true) || (SON == true))) {     //If button config has changed AND one of the LED sets are on
    MixColor();
    MixLEDChange = false;
  } 
  
  if ((MixLEDChange == true) && ((HgON ==false) && (ClON == false) && (SON == false))){   //If button config has changed AND none of the LEDs are on
    fill_solid(MixLEDStrip, MixLEDNum, CRGB::Black);
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgState == HIGH) && (HgON == false)) {   //check for change in Hg/Cl/S switches
    HgAdd();
  }

  if ((HgState == LOW) && (HgON == true)) {
    HgSub();
  }

  if ((ClState == HIGH) && (ClON == false)) {
    ClAdd();
  }

  if ((ClState == LOW) && (ClON == true)) {
    ClSub();
  }

  if ((SState == HIGH) && (SON == false)) {
    SAdd();
  }

  if ((SState == LOW) && (SON == true)) {
    SSub();
  }

  if ((MixState == HIGH)&& (Step == 0)) {   //if mix button is pressed, check step and advance or warn
    FirstStep();
    return;                     //return statement is necessary or the loop will continue before re-reading the button states, causing false positive on next step
  }

  if ((MixState == HIGH)&& (Step == 1)) {
    SecondStep();
    return;
  }

  if ((MixState == HIGH)&& (Step == 2)) {
    ThirdStep();
    return;
  }

  if ((MixState == HIGH)&& (Step == 3)) {
    FourthStep();
    return;
  }

  if ((MixState == HIGH)&& (Step == 4)) {
    FifthStep();
    return;
  }

  if ((MixState == HIGH)&& (Step == 5)) {
    SixthStep();
    return;
  }
  
}

void reset() {

  Step = 0;                     //reset all variables, lights, and play reset sound
  Warning = 0;
  HotPotPos = 0;
  HgON = false;
  ClON = false;
  SON = false;
  HotPotChng = 0;
  ShutdownSequence = false;
  MixLEDChange = false;

  fill_solid(HgLEDStrip, HgLEDNum, CRGB::Black);
  fill_solid(ClLEDStrip, ClLEDNum, CRGB::Black);
  fill_solid(SLEDStrip, SLEDNum, CRGB::Black);
  fill_solid(HotLEDStrip, HotLEDNum, CRGB::Black);
  fill_solid(MixLEDStrip, MixLEDNum, CRGB::Black);
  fill_solid(ProgLEDStrip, ProgLEDNum, CRGB::Black);
  FastLED.show();

  digitalWrite(WarnLED1, LOW);
  digitalWrite(WarnLED2, LOW);
  digitalWrite(Lock, LOW);

  Speaker.play(1);

  delay(5000);            //delay is necessary to freeze until sound is done playing
  
}

void HgAdd() {

  fill_solid(HgLEDStrip, HgLEDNum, CRGB::Blue);   //when switch state change detected, adjust lights, sounds, and variables
  FastLED.show();
  Speaker.play(8);
  HgON = true;
  delay(500);
  MixLEDChange = true;
}

void HgSub() {

  fill_solid(HgLEDStrip, HgLEDNum, CRGB::Black);
  FastLED.show();
  Speaker.play(8);
  HgON = false;
  delay(500);
  MixLEDChange = true;
}

void ClAdd() {

  fill_solid(ClLEDStrip, ClLEDNum, CRGB::Green);
  FastLED.show();
  Speaker.play(9);
  ClON = true;
  delay(500);
  MixLEDChange = true;
}

void ClSub() {

  fill_solid(ClLEDStrip, ClLEDNum, CRGB::Black);
  FastLED.show();
  Speaker.play(9);
  ClON = false;
  delay(500);
  MixLEDChange = true;
}

void SAdd() {

  fill_solid(SLEDStrip, SLEDNum, CRGB::Yellow);
  FastLED.show();
  Speaker.play(10);
  SON = true;
  delay(500);
  MixLEDChange = true;
}

void SSub() {

  fill_solid(SLEDStrip, SLEDNum, CRGB::Black);
  FastLED.show();
  Speaker.play(10);
  SON = false;
  delay(500);
  MixLEDChange = true;
}

void MixColor() {         //the colors to display in the "mixing" container, based on which switches are on

  if ((HgON == true) && (ClON == false) && (SON == false)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (0, 0, 255));
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgON == true) && (ClON == true) && (SON == false)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (0, 255, 255));
    FastLED.show();
    MixLEDChange = false;
  } 

  if ((HgON == true) && (ClON == true) && (SON == true)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (150, 255 , 255));
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgON == false) && (ClON == true) && (SON == false)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (0, 255, 0));
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgON == false) && (ClON == true) && (SON == true)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (175, 255, 0));
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgON == false) && (ClON == false) && (SON == true)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (255, 220, 0));
    FastLED.show();
    MixLEDChange = false;
  }

  if ((HgON == true) && (ClON == false) && (SON == true)) {
    fill_solid(MixLEDStrip, MixLEDNum, CRGB (100, 255, 100));
    FastLED.show();
    MixLEDChange = false;
  }
}

void HotPotColor() {      //if the position of the potentiometer changes, change the LED color

  if (HotPotPos == 0) {
    fill_solid(HotLEDStrip, HotLEDNum, CRGB::Black);
    FastLED.show();
    Speaker.play(11);
  }

  if (HotPotPos == 1) {
    fill_solid(HotLEDStrip, HotLEDNum, CRGB (150, 10, 0));
    FastLED.show();
    Speaker.play(11);
  }

  if (HotPotPos == 2) {
    fill_solid(HotLEDStrip, HotLEDNum, CRGB (255, 50, 0));
    FastLED.show();
    Speaker.play(11);
  }
  
}

void HotPotRead() {

  HotPotNum = analogRead(HotPot);       //Read the position of the potentiometer
  HotPotNum = (HotPotNum + ((analogRead(HotPot)) + (analogRead(HotPot)) + (analogRead(HotPot)))/4);    //Create a more general, average reading

  if (HotPotNum <= 500) {         //assign a position of 0-2 for the potentiometer for simplicity, since only 3 positions are needed
    HotPotPos = 0;
  }

  if ((HotPotNum > 500)&&(HotPotNum <=1200)) {
    HotPotPos = 1;
  }

  if (HotPotNum > 1200) {
    HotPotPos = 2;
  }

  if (HotPotPos != HotPotChng) {          //if there is a change in position, change the color
    HotPotColor();
    HotPotChng = HotPotPos;
  }
  
}

void FirstStep() {

  if ((HgON == true) && (ClON == false) && (SON == true) && (HotPotPos == 1)) {     //requirements for the first step -- if met, advance. If not, play a warning
    Speaker.play(2);
    delay(1000);
    ProgLEDStrip[0] = CRGB::Green;
    FastLED.show();
    Step = 1;
    delay(1000);
    return;
  } else {
  Warn();
  Serial.println("First step");
  }
}

void SecondStep() {

  if ((HgON == false) && (ClON == true) && (SON == true) && (HotPotPos == 0)) {
    Speaker.play(2);
    delay(1000);
    ProgLEDStrip[1] = CRGB::Green;
    FastLED.show();
    Step = 2;
    delay(1000);
    return;
  } else{
    Warn();
    Serial.println("Second step");
  }
}

void ThirdStep() {

  if ((HgON == true) && (ClON == true) && (SON == false) && (HotPotPos == 2)) {
    Speaker.play(2);
    delay(1000);
    ProgLEDStrip[2] = CRGB::Green;
    FastLED.show();
    Step = 3;
    delay(1000);
  } else {
    Warn();
    Serial.println("Third step");
  }
}

void FourthStep() {

  if ((HgON == true) && (ClON == true) && (SON == true) && (HotPotPos == 0)) {
    Speaker.play(2);
    delay(1000);
    ProgLEDStrip[3] = CRGB::Green;
    FastLED.show();
    Step = 4;
    delay(1000);
    return;
  } else {
    Warn();
  }
}

void FifthStep() {

  if ((HgON == true) && (ClON == false) && (SON == false) && (HotPotPos == 2)) {
    Speaker.play(2);
    delay(1000);
    ProgLEDStrip[4] = CRGB::Green;
    FastLED.show();
    Step = 5;
    delay(1000);
    return;
  } else {
    Warn();
  }
}

void SixthStep() {

  if ((HgON == false) && (ClON == false) && (SON == true) && (HotPotPos == 1)) {
    Speaker.play(3);
    delay(1000);
    ProgLEDStrip[5] = CRGB::Green;
    FastLED.show();
    Step = 6;
    delay(200);
    digitalWrite(Lock, HIGH);
    digitalWrite(WarnLED1, LOW);
    digitalWrite(WarnLED2, LOW);
    return;
  } else {
    Warn();
  }
}

void Warn() {

  if (Warning == 0) {
    Speaker.play(4);
    delay(2000);
    digitalWrite(WarnLED1, HIGH);
    Warning = 1;
    delay(200);
    return;
  }

  if (Warning == 1) {
    Speaker.play(5);
    delay(2000);
    digitalWrite(WarnLED2, HIGH);
    Warning = 2;
    delay(200);
    return;
  }

  if (Warning == 2) {
    Speaker.play(6);
    delay(2000);
    Shutdown();
    delay(200);
    return;
  }
  
}

void Shutdown() {

  fill_solid(ProgLEDStrip, ProgLEDNum, CRGB::Red);
  fill_solid(MixLEDStrip, MixLEDNum, CRGB::Red);
  fill_solid(HgLEDStrip, HgLEDNum, CRGB::Red);
  fill_solid(ClLEDStrip, ClLEDNum, CRGB::Red);
  fill_solid(SLEDStrip, SLEDNum, CRGB::Red);
  fill_solid(HotLEDStrip, HotLEDNum, CRGB::Red);
  FastLED.show();
  ShutdownSequence = true;
}
