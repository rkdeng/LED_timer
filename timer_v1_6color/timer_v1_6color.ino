// for led
#include <FastLED.h> // load the fastled library
const int LED_PIN = 8;  // led signal pin
const int NUM_LEDS = 12, totalColor = 6; // total number of led
CRGB leds[NUM_LEDS]; // for FastLED
// value for different colors, convention: 1st row is black, then followed by (totalColor) color choices
int myColor[7][3] = {
  {0, 0, 0},
  {0, 231, 0},
  {140, 255, 0},
  {239, 255, 0},
  {129, 0, 31},
  {68, 0, 255},
  {0, 118, 137}
};

int nLED = 1, colorNum = 1, dispColor; // global variable to store led number and color for display
bool blinkState = true;  // for blinking
double bright = 0.7, brightPre = 0.05, brightDisp;  // brightness
int preLED = 1; // store previous stage for the led
volatile unsigned long blinkTic, blinkInter = 1000; // for blinking led

// blink leds when finished
unsigned long blink_p3, inter_p3 = 500;
bool state_p3 = true;
long colorTmp;

// pins for rotary encoder, all are pulled up through with 10k resistor
const int clkPin = 6; //the clk attach to pin 6
const int dtPin = 7; //the dt pin attach to pin 7
const int swPin = 2; //switch pin

// button interrupt function
volatile int menuPage = 1; // switch from 3 states
volatile unsigned long lastInterrupt;
int encoderVal = 0; // encoder value

int totalTime = 1; // current total time, in minute

// countdown timer
volatile unsigned long startTime, extraTime = 0, interTime;

// buzzer
#include "pitches.h"  // load pitch library
int buzPin = 9;  // buzzer pin
unsigned long timeTic;
int thisNote, buzzState, noteDuration, buzzN; // buzzN control the number of time it buzz
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4, 0
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4, 1
};


void setup()
{
  // led setup
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);

  //initial led
  for (int i = 0; i <= (NUM_LEDS - 1); i++)
  {
    leds[i] = CRGB(0, 0, 0);
    FastLED.show();
  }

  randomSeed(analogRead(1));
  // start animation
  for (int i = 0; i <= (NUM_LEDS - 1); i++)
  {
    int iniColor = random(1, (totalColor + 1));
    leds[i] = CRGB(myColor[iniColor][0] * bright, myColor[iniColor][1] * bright, myColor[iniColor][2] * bright);
    FastLED.show();
    delay(300);
  }
  for (int i = (NUM_LEDS - 1); i > -1; i--)
  {
    leds[i] = CRGB(0, 0, 0);
    FastLED.show();
    delay(30);
  }
  // light up the 1st LED as indication of 1 time unit in the initial state
  leds[0] = CRGB(myColor[1][0] * bright, myColor[1][1] * bright, myColor[1][2] * bright);
  FastLED.show();

  //set clkPin,dePin,swPin as INPUT
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT);

  //switch interrupt
  attachInterrupt(digitalPinToInterrupt(swPin), swpress, FALLING);

  // buzzer

  Serial.begin(9600); // initialize serial communications at 9600 bps

}

void loop()
{
  int change;

  switch (menuPage)
  {
    case 1:
      //            Serial.print("setting page");
      //            Serial.print("\n");


      // monitor the rotary encoder

      change = getEncoderTurn();//

      // transform encoder input to led number
      totalTime = totalTime + change;
      if (totalTime < 1)
      {
        totalTime = 1;
      }

      //Serial.println(totalTime);
      // display led
      time_to_led(totalTime);

      //Serial.println(nLED);
      if (nLED != preLED)
      {
        preLED = nLED;
        
        //Serial.print("n led:");
        //Serial.println(nLED);
       // Serial.print("color num");
        //Serial.println(colorNum);

        dispLED();
      }


      break;

    case 2:
      //Serial.print("countdown page");
      //Serial.print("\n");

      //Serial.println((millis() - startTime));
      //Serial.println(extraTime);

      // countdown timer
      if (extraTime != 0)
      {
        interTime = extraTime;
        //Serial.println(interTime);
        //Serial.println(extraTime);
        //Serial.println(interTime);
      } else {
        interTime = 10000;
      }

      //Serial.println(interTime);

      if (millis() - startTime >= interTime)
      {
        //Serial.println(interTime);
        totalTime = totalTime - 1;
        startTime = millis();

      }

      // display led
      time_to_led(totalTime);

      // when 1 time tic is finished, update all leds once
      if (nLED != preLED)
      {
        extraTime = 0; // update extra time
        preLED = nLED;
        // Serial.print("update");
        //Serial.print("n led:");
        //Serial.println(nLED);
        //Serial.print("color num");
        dispLED();
      }

      // blink the latest LED
      if (nLED == preLED)
      {
        //Serial.println(millis() - blinkTic);
        if (millis() - blinkTic >= blinkInter)
        {
          //Serial.print("blink");
          //Serial.print("\n");
          //Serial.println(millis() - blinkTic);
          if (blinkState)
          {
            leds[(nLED - 1)] = CRGB(0, 0, 0);
            FastLED.show();
            blinkState = false;
          } else if (~blinkState)
          {

            leds[(nLED - 1)] = CRGB(myColor[colorNum][0] * bright, myColor[colorNum][1] * bright, myColor[colorNum][2] * bright);
            FastLED.show();
            blinkState = true;
          }
          blinkTic = millis();
        }
      }

      // when countdown finish, go to buzz page
      if (totalTime <= 0)
      {
        menuPage = 3;
        blink_p3 = millis();

        // initialize them for buzzer
        thisNote = 0;
        buzzState = 0;
        buzzN = 5; // change the number of buzzer melody
      }

      break;

    case 3:
      //Serial.print("buzz page");
      //Serial.print("\n");

      if ((millis() - blink_p3) >= inter_p3)
      {
        if (state_p3)
        {
          // generate random color from my color
          colorTmp = random(1, (totalColor + 1));

          for (int i = 0; i <= (NUM_LEDS - 1); i++)
          {
            leds[i] = CRGB(myColor[colorTmp][0] * bright, myColor[colorTmp][1] * bright, myColor[colorTmp][2] * bright);
            // leds[0] = CRGB(60, 0, 0);
            FastLED.show();
          }
          state_p3 = false;
        } else if (~state_p3) {
          for (int i = 0; i <= (NUM_LEDS - 1); i++)
          {
            leds[i] = CRGB(0, 0, 0);
            FastLED.show();
          }
          state_p3 = true;

        }
        blink_p3 = millis();
      }

      // buzz for 30 seconds

      if (buzzN > 0)
      {
        switch (buzzState)
        {
          // turn on a sound if there is no sound
          case 0:
            noteDuration = 1000 / noteDurations[thisNote];  // calculate note duration
            if (melody[thisNote] != 0)
            {
              tone(buzPin, melody[thisNote], noteDuration);  // turn on buzzer
            }
            buzzState = 1;
            timeTic = millis();
            break;
          // turn of the sound when one melody is finished
          case 1:
            if ((millis() - timeTic) >= noteDuration * 1.3)
            {
              //Serial.println(noteDuration);
              noTone(buzPin);
              //timeTic = millis();
              buzzState = 0;
              if (thisNote == 8)
              {
                thisNote = 0;
                buzzN = buzzN - 1;
              } else {
                thisNote = thisNote + 1;
              }

            }
            break;

        }
      }
      break;
  }

  //Serial.print(menupage);

  // trigger buzzer when time is up

}

// function to switch states when button is pressed.
void swpress()
{
  if (millis() - lastInterrupt > 250) // no-interrupt time window to debounce
  {
    switch (menuPage)
    {
      case 1: // change from setting to countdown
        menuPage = 2;
        startTime = millis();
        blinkTic = millis();
        //Serial.print("now page 2");
        //Serial.print("\n");
        break;
      case 2: // change from countdown to selection
        menuPage = 1;
        extraTime = interTime - millis() + startTime;
        dispLED();
        //Serial.println(extraTime);
       // Serial.print("now page 1");
       // Serial.print("\n");
        break;
      case 3: // change from buzzing to selection
        menuPage = 1;
        break;
    }

    lastInterrupt = millis();
  }
}

// function to count the change of rotary encoder
int getEncoderTurn(void)
{
  static int oldA = HIGH; //set the oldA as HIGH
  static int oldB = HIGH; //set the oldB as HIGH
  int result = 0;
  int newA = digitalRead(clkPin);//read the value of clkPin to newA
  int newB = digitalRead(dtPin);//read the value of dtPin to newB
  if (newA != oldA || newB != oldB) //if the value of clkPin or the dtPin has changed
  {
    // something has changed
    if (oldA == HIGH && newA == LOW)
    {
      result = (oldB * 2 - 1);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
}

// functions to display leds

// calculate number of led and color from total time
void time_to_led(int ttime)
{
  int cNum_tmp;
  cNum_tmp = (ttime - 1) / NUM_LEDS + 1; // color

  colorNum = cNum_tmp % totalColor;
  if (colorNum == 0)
  {
    colorNum = totalColor;
  }

  nLED = ttime % NUM_LEDS; // number of led to display
  if (nLED == 0)
  {
    nLED = NUM_LEDS;
  }
}


// update LEDs, need to calculate nLED and colorNum first using time_to_led function
void dispLED()
{
  for (int i = 0; i <= (NUM_LEDS - 1); i++)
  {
    if (i <= (nLED - 1))
    {
      dispColor = colorNum;
      brightDisp = bright;
    } else
    {
      dispColor = colorNum - 1;
      //Serial.println(totalTime);
      brightDisp = brightPre;
      if (totalTime > 12 && dispColor == 0)
      {
        dispColor = totalColor;
      }
    }
    leds[i] = CRGB(myColor[dispColor][0] * brightDisp, myColor[dispColor][1] * brightDisp, myColor[dispColor][2] * brightDisp);
    FastLED.show();
  }
}
