/*
 * Author: https://github.com/karasikq/
 * Telegram: @cbate
 * License: GNU GPLv3
*/

#define FHT_N 64
#define LOG_OUT 1

#include <FHT.h>
#include <EEPROMex.h>
#include "FastLED.h"
#include "IRLremote.h"
#include "Bluetooth.h"

CHashIR IRLremote;
Bluetooth blue(3,4);
uint32_t IRdata;
#define BUTT_UP     0xF39EEBAD
#define BUTT_DOWN   0xC089F6AD
#define BUTT_LEFT   0xE25410AD
#define BUTT_RIGHT  0x14CE54AD
#define BUTT_OK     0x297C76AD
#define BUTT_1      0x4E5BA3AD
#define BUTT_2      0xE51CA6AD
#define BUTT_3      0xE207E1AD
#define BUTT_4      0x517068AD
#define BUTT_5      0x1B92DDAD
#define BUTT_6      0xAC2A56AD
#define BUTT_7      0x5484B6AD
#define BUTT_8      0xD22353AD
#define BUTT_9      0xDF3F4BAD
#define BUTT_0      0xF08A26AD
#define BUTT_STAR   0x68E456AD
#define BUTT_HASH   0x151CD6AD

#define numOfLeds 60
#define FASTLED_ALLOW_INTERRUPTS 1
const bool saveSettings = 1;
const bool resetSettings = 0;
byte brightness = 255;
CRGB leds[numOfLeds];
char* BluetoothBuffer;

// Pins
#define potGndPin A0
#define channelL A1
#define channelR A2
#define channelRFreq A3
#define remotePin 2
#define ledPin 12

#define numOfModes 9
#define waitTime 5
#define monoSignal 1
#define EXP 1.4
#define backgroundColor HUE_PURPLE

#define autoLowFilter 0
#define saveLowFilter 1
#define additionalFilter 3
#define additionalFreqFilter 3

int subInt = 0;

// Mode
float smoothRatio = 0.5;
#define loudRatio 1.8 

// Mode
float freqSmoothRatio = 0.8;
float sensitivityRatio = 1.2;
#define decreaseStep 20
#define lowFreqColor HUE_RED
#define midFreqColor HUE_GREEN
#define highFreqColor HUE_YELLOW

// Mode
#define strobeSat 255
int strobePeriod = 100;
byte strobeFluency = 100;
byte strobeColor = 0;

// Mode
byte lightColor = 0;
byte lightSat = 200;
byte lightBright = 255;
byte colorChangingSpeed = 100;
int rainbowPeriod = 3;
float rainbowDist_2 = 5.5;

// Mode
byte runFreqSpeed = 60;

// Mode 
byte colorStart = 0;
byte colorStep = 5;
#define colorSmooth 2

#define ledPiece numOfLeds / 5
float freq_to_stripe = numOfLeds / 40;

DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

byte channelRlenght, channelLlenght;
float channelRlvl, channelRElvl;
float channelLlvl, channelLElvl;

byte backgroundBright = 30;
float rainbowDist = 5.5;

uint16_t lowFilter = 100; 
uint16_t lowFilter2 = 40;

float averageLevel = 50;
float averK = 0.009;
int maxLevel = 100;
byte MAX_CH = numOfLeds / 2;
int hue;
unsigned long loopTimer, strobeTimer, runTimer, changeColorTimer, rainbowTimer, memoryTimer;
byte count;
float index = (float)255 / MAX_CH;
bool lowFlag;
byte low_pass;
int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
float colorMusic_f[3], colorMusic_aver[3];
bool colorMusicFlash[3], strobeUp_flag, strobeDwn_flag;
byte currentMode = 0;
int thisBright[3], strobe_bright = 0;
unsigned int light_time = strobePeriod * 0.2;
volatile bool ir_flag;
bool settings_mode, ONstate = true;
int8_t freq_strobe_mode, light_mode;
int freq_max;
float freq_max_f, rainbowStep;
int freq_f[32];
int this_color;
bool running_flag[3], eeprom_flag;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

void setup() {
  Serial.begin(9600);
  blue.begin();
  blue.setBaudrate(7);
  blue.reset();
  FastLED.addLeds<WS2811, ledPin, GRB>(leds, numOfLeds).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(brightness);
  Serial.setTimeout(50);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(potGndPin, OUTPUT);
  digitalWrite(potGndPin, LOW);
  IRLremote.begin(remotePin);
  analogReference(EXTERNAL);

  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  if (resetSettings)
    EEPROM.write(100, 0);

  if (autoLowFilter && !saveLowFilter)
    autoLowPass();
  if (saveLowFilter)
  {
    lowFilter = EEPROM.readInt(70);
    lowFilter2 = EEPROM.readInt(72);
  }
  if (saveSettings) 
    if (EEPROM.read(100) != 100)
    {
      EEPROM.write(100, 100);
      updateEEPROM();
    }
    else
      readEEPROM();
}
long lm = millis();
void loop() 
{
  bluetoothTick();
  remoteTick();
  calculate();
  eepromTick();
}

void calculate() 
{
  if (ONstate)
  {
    if (millis() - loopTimer > waitTime)
    {
      channelRlvl = 0;
      channelLlvl = 0;

      if (currentMode == 0 || currentMode == 1) {
        for (byte i = 0; i < 100; i ++) {
          RcurrentLevel = analogRead(channelR);
          if (!monoSignal)
            LcurrentLevel = analogRead(channelL);
          if (channelRlvl < RcurrentLevel)
            channelRlvl = RcurrentLevel;
          if (!monoSignal)
            if (channelLlvl < LcurrentLevel) channelLlvl = LcurrentLevel;
        }

        channelRlvl = map(channelRlvl, lowFilter, 1023, 0, 500);
        if (!monoSignal)
          channelLlvl = map(channelLlvl, lowFilter, 1023, 0, 500);
        channelRlvl = constrain(channelRlvl, 0, 500);
        if (!monoSignal)
          channelLlvl = constrain(channelLlvl, 0, 500);

        channelRlvl = pow(channelRlvl, EXP);
        if (!monoSignal)
          channelLlvl = pow(channelLlvl, EXP);

        channelRElvl = channelRlvl * smoothRatio + channelRElvl * (1 - smoothRatio);
        if (monoSignal) 
          channelLElvl = channelRElvl;
        else
          channelLElvl = channelLlvl * smoothRatio + channelLElvl * (1 - smoothRatio);

        if (backgroundBright > 10)
          for (int i = 0; i < numOfLeds; i++)
            leds[i] = CHSV(backgroundColor, 255, backgroundBright);

        if (channelRElvl > 10 && channelLElvl > 10) 
        {
          averageLevel = (float)(channelRElvl + channelLElvl) / 2 * averK + averageLevel * (1 - averK);
          maxLevel = (float)averageLevel * loudRatio;
          channelRlenght = map(channelRElvl, 0, maxLevel, 0, MAX_CH);
          channelLlenght = map(channelLElvl, 0, maxLevel, 0, MAX_CH);
          channelRlenght = constrain(channelRlenght, 0, MAX_CH);
          channelLlenght = constrain(channelLlenght, 0, MAX_CH);
          draw();
        }
      }

      if (currentMode == 2 || currentMode == 3 || currentMode == 4 || currentMode == 7 || currentMode == 8) 
      {
        analyzeAudio();
        colorMusic[0] = 0;
        colorMusic[1] = 0;
        colorMusic[2] = 0;
        for (int i = 0 ; i < 32 ; i++)
          if (fht_log_out[i] < lowFilter2) fht_log_out[i] = 0;

        for (byte i = 2; i < 4; i++)
          if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];

        for (byte i = 4; i < 8; i++)
          if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];

        for (byte i = 8; i < 32; i++)
          if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
          
        freq_max = 0;
        for (byte i = 0; i < 30; i++)
        {
          if (fht_log_out[i + 2] > freq_max) freq_max = fht_log_out[i + 2];
          if (freq_max < 5) freq_max = 5;
          if (freq_f[i] < fht_log_out[i + 2]) freq_f[i] = fht_log_out[i + 2];
          if (freq_f[i] > 0) freq_f[i] -= colorSmooth;
          else freq_f[i] = 0;
        }
        freq_max_f = freq_max * averK + freq_max_f * (1 - averK);
        for (byte i = 0; i < 3; i++)
        {
          colorMusic_aver[i] = colorMusic[i] * averK + colorMusic_aver[i] * (1 - averK);
          colorMusic_f[i] = colorMusic[i] * freqSmoothRatio + colorMusic_f[i] * (1 - freqSmoothRatio);
          if (colorMusic_f[i] > ((float)colorMusic_aver[i] * sensitivityRatio))
          {
            thisBright[i] = 255;
            colorMusicFlash[i] = true;
            running_flag[i] = true;
          } else 
            colorMusicFlash[i] = false;
          if (thisBright[i] >= 0)
            thisBright[i] -= decreaseStep;
          if (thisBright[i] < backgroundBright)
          {
            thisBright[i] = backgroundBright;
            running_flag[i] = false;
          }
        }
        draw();
      }
      if (currentMode == 5) 
      {
        if ((long)millis() - strobeTimer > strobePeriod) {
          strobeTimer = millis();
          strobeUp_flag = true;
          strobeDwn_flag = false;
        }
        if ((long)millis() - strobeTimer > light_time)
          strobeDwn_flag = true;

        if (strobeUp_flag)
        {
          if (strobe_bright < 255)
            strobe_bright += strobeFluency;
          if (strobe_bright > 255)
          {
            strobe_bright = 255;
            strobeUp_flag = false;
          }
        }

        if (strobeDwn_flag)
        {
          if (strobe_bright > 0)
            strobe_bright -= strobeFluency;
          if (strobe_bright < 0)
          {
            strobeDwn_flag = false;
            strobe_bright = 0;
          }
        }
        draw();
      }
      if (currentMode == 6)
        draw();

      if (!IRLremote.receiving())
        FastLED.show();
      if (currentMode != 7)
        FastLED.clear();
      loopTimer = millis();
    }
  }
}

void draw()
{
  switch (currentMode)
  {
    case 0:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - channelRlenght); i--)
      {
        leds[i] = ColorFromPalette(myPal, (count * index));
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + channelLlenght); i++ )
      {
        leds[i] = ColorFromPalette(myPal, (count * index));
        count++;
      }
      if (backgroundBright > 0)
      {
        CHSV this_dark = CHSV(backgroundColor, 255, backgroundBright);
        for (int i = ((MAX_CH - 1) - channelRlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + channelLlenght; i < numOfLeds; i++)
          leds[i] = this_dark;
      }
      break;
    case 1:
      if (millis() - rainbowTimer > 30)
      {
        rainbowTimer = millis();
        hue = floor((float)hue + rainbowDist);
      }
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - channelRlenght); i--)
      {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + channelLlenght); i++ )
      {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);
        count++;
      }
      if (backgroundBright > 0)
      {
        CHSV this_dark = CHSV(backgroundColor, 255, backgroundBright);
        for (int i = ((MAX_CH - 1) - channelRlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + channelLlenght; i < numOfLeds; i++)
          leds[i] = this_dark;
      }
      break;
    case 2:
      for (int i = 0; i < numOfLeds; i++)
      {
        if (i < ledPiece)          leds[i] = CHSV(highFreqColor, 255, thisBright[2]);
        else if (i < ledPiece * 2) leds[i] = CHSV(midFreqColor, 255, thisBright[1]);
        else if (i < ledPiece * 3) leds[i] = CHSV(lowFreqColor, 255, thisBright[0]);
        else if (i < ledPiece * 4) leds[i] = CHSV(midFreqColor, 255, thisBright[1]);
        else if (i < ledPiece * 5) leds[i] = CHSV(highFreqColor, 255, thisBright[2]);
      }
      break;
    case 3:
      for (int i = 0; i < numOfLeds; i++)
      {
        if (i < numOfLeds / 3)          leds[i] = CHSV(highFreqColor, 255, thisBright[2]);
        else if (i < numOfLeds * 2 / 3) leds[i] = CHSV(midFreqColor, 255, thisBright[1]);
        else if (i < numOfLeds)         leds[i] = CHSV(lowFreqColor, 255, thisBright[0]);
      }
      break;
    case 4:
      switch (freq_strobe_mode)
      {
        case 0:
          if (colorMusicFlash[2]) HIGHS();
          else if (colorMusicFlash[1]) MIDS();
          else if (colorMusicFlash[0]) LOWS();
          else SILENCE();
          break;
        case 1:
          if (colorMusicFlash[2]) HIGHS();
          else SILENCE();
          break;
        case 2:
          if (colorMusicFlash[1]) MIDS();
          else SILENCE();
          break;
        case 3:
          if (colorMusicFlash[0]) LOWS();
          else SILENCE();
          break;
      }
      break;
    case 5:
      if (strobe_bright > 0)
      {
        for (int i = 0; i < numOfLeds; i++)
          leds[i] = CHSV(strobeColor, strobeSat, strobe_bright);
        if(strobeColor < 255)
          strobeColor += 15;
        else
          strobeColor = 0;
      }
      else
        for (int i = 0; i < numOfLeds; i++)
          leds[i] = CHSV(backgroundColor, 255, backgroundBright);
      break;
    case 6:
      switch (light_mode)
      {
        case 0:
          for (int i = 0; i < numOfLeds; i++)
            leds[i] = CHSV(lightColor, lightSat, lightBright);
          break;
        case 1:
          if (millis() - changeColorTimer > colorChangingSpeed)
          {
            changeColorTimer = millis();
            if (++this_color > 255) this_color = 0;
          }
          for (int i = 0; i < numOfLeds; i++)
            leds[i] = CHSV(this_color, lightSat, lightBright);
          break;
        case 2:
          if (millis() - rainbowTimer > 30)
          {
            rainbowTimer = millis();
            this_color += rainbowPeriod;
            if (this_color > 255) this_color = 0;
            if (this_color < 0) this_color = 255;
          }
          rainbowStep = this_color;
          for (int i = 0; i < numOfLeds; i++)
          {
            leds[i] = CHSV((int)floor(rainbowStep), 255, lightBright);
            rainbowStep += rainbowDist_2;
            if (rainbowStep > 255) rainbowStep = 0;
            if (rainbowStep < 0) rainbowStep = 255;
          }
          break;
      }
      break;
    case 7:
      switch (freq_strobe_mode)
      {
        case 0:
          if (running_flag[2])
            leds[numOfLeds / 2] = CHSV(highFreqColor, 255, thisBright[2]);
          else
            if (running_flag[1])
              leds[numOfLeds / 2] = CHSV(midFreqColor, 255, thisBright[1]);
            else
              if (running_flag[0])
                leds[numOfLeds / 2] = CHSV(lowFreqColor, 255, thisBright[0]);
              else
                leds[numOfLeds / 2] = CHSV(backgroundColor, 255, backgroundBright);
          break;
        case 1:
          if (running_flag[2]) leds[numOfLeds / 2] = CHSV(highFreqColor, 255, thisBright[2]);
          else leds[numOfLeds / 2] = CHSV(backgroundColor, 255, backgroundBright);
          break;
        case 2:
          if (running_flag[1]) leds[numOfLeds / 2] = CHSV(midFreqColor, 255, thisBright[1]);
          else leds[numOfLeds / 2] = CHSV(backgroundColor, 255, backgroundBright);
          break;
        case 3:
          if (running_flag[0]) leds[numOfLeds / 2] = CHSV(lowFreqColor, 255, thisBright[0]);
          else leds[numOfLeds / 2] = CHSV(backgroundColor, 255, backgroundBright);
          break;
      }
      leds[(numOfLeds / 2) - 1] = leds[numOfLeds / 2];
      if (millis() - runTimer > runFreqSpeed) {
        runTimer = millis();
        for (byte i = 0; i < numOfLeds / 2 - 1; i++) {
          leds[i] = leds[i + 1];
          leds[numOfLeds - i - 1] = leds[i];
        }
      }
      break;
    case 8:
      byte HUEindex = colorStart;
      for (byte i = 0; i < numOfLeds / 2; i++) {
        byte this_bright = map(freq_f[(int)floor((numOfLeds / 2 - i) / freq_to_stripe)], 0, freq_max_f, 0, 255);
        this_bright = constrain(this_bright, 0, 255);
        leds[i] = CHSV(HUEindex, 255, this_bright);
        leds[numOfLeds - i - 1] = leds[i];
        HUEindex += colorStep;
        if (HUEindex > 255) HUEindex = 0;
      }
      break;
  }
}

void HIGHS() {
  for (int i = 0; i < numOfLeds; i++) leds[i] = CHSV(highFreqColor, 255, thisBright[2]);
}
void MIDS() {
  for (int i = 0; i < numOfLeds; i++) leds[i] = CHSV(midFreqColor, 255, thisBright[1]);
}
void LOWS() {
  for (int i = 0; i < numOfLeds; i++) leds[i] = CHSV(lowFreqColor, 255, thisBright[0]);
}
void SILENCE() {
  for (int i = 0; i < numOfLeds; i++) leds[i] = CHSV(backgroundColor, 255, backgroundBright);
}


int limitIncrInt(int value, int incr_step, int mininmum, int maximum)
{
  int val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

float limitIncrFloat(float value, float incr_step, float mininmum, float maximum)
{
  float val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

void remoteTick()
{
  if (IRLremote.available())
  {
    auto data = IRLremote.read();
    IRdata = data.command;
    ir_flag = true;
  }
  if (ir_flag)
  {
    memoryTimer = millis();
    eeprom_flag = true;
    switch (IRdata)
    {
      case BUTT_1: currentMode = 0;
        break;
      case BUTT_2: currentMode = 1;
        break;
      case BUTT_3: currentMode = 2;
        break;
      case BUTT_4: currentMode = 3;
        break;
      case BUTT_5: currentMode = 4;
        break;
      case BUTT_6: currentMode = 5;
        break;
      case BUTT_7: currentMode = 6;
        break;
      case BUTT_8: currentMode = 7;
        break;
      case BUTT_9: currentMode = 8;
        break;
      case BUTT_0: fullLowPass();
        break;
      case BUTT_STAR: 
        ONstate = !ONstate;
        FastLED.clear();
        FastLED.show();
        updateEEPROM();
        break;
      case BUTT_HASH:
        switch (currentMode) {
          case 4:
          case 7: 
            if (++freq_strobe_mode > 3)
              freq_strobe_mode = 0;
            break;
          case 6:
            if (++light_mode > 2)
              light_mode = 0;
            break;
        }
        break;
      case BUTT_OK:
        settings_mode = !settings_mode;
        digitalWrite(13, settings_mode);
        break;
      case BUTT_UP:
        if (settings_mode)
          backgroundBright = limitIncrInt(backgroundBright, 5, 0, 255);
        else
        {
          switch (currentMode)
          {
            case 0:
              break;
            case 1: rainbowDist = limitIncrFloat(rainbowDist, 0.5, 0.5, 20);
              break;
            case 2:
            case 3:
            case 4: sensitivityRatio = limitIncrFloat(sensitivityRatio, 0.1, 0, 5);
              break;
            case 5: strobePeriod = limitIncrInt(strobePeriod, 20, 1, 1000);
              break;
            case 6:
              switch (light_mode) {
                case 0: lightSat = limitIncrInt(lightSat, 20, 0, 255);
                  break;
                case 1: lightSat = limitIncrInt(lightSat, 20, 0, 255);
                  break;
                case 2: rainbowDist_2 = limitIncrFloat(rainbowDist_2, 0.5, 0.5, 10);
                  break;
              }
              break;
            case 7: sensitivityRatio = limitIncrFloat(sensitivityRatio, 0.1, 0.0, 10);
              break;
            case 8: colorStart = limitIncrInt(colorStart, 10, 0, 255);
              break;
          }
        }
        break;
      case BUTT_DOWN:
        if (settings_mode) {
          // ВНИЗ общие настройки
          backgroundBright = limitIncrInt(backgroundBright, -5, 0, 255);
        } else {
          switch (currentMode) {
            case 0:
              break;
            case 1: rainbowDist = limitIncrFloat(rainbowDist, -0.5, 0.5, 20);
              break;
            case 2:
            case 3:
            case 4: sensitivityRatio = limitIncrFloat(sensitivityRatio, -0.1, 0, 5);
              break;
            case 5: strobePeriod = limitIncrInt(strobePeriod, -20, 1, 1000);
              break;
            case 6:
              switch (light_mode) {
                case 0: lightSat = limitIncrInt(lightSat, -20, 0, 255);
                  break;
                case 1: lightSat = limitIncrInt(lightSat, -20, 0, 255);
                  break;
                case 2: rainbowDist_2 = limitIncrFloat(rainbowDist_2, -0.5, 0.5, 10);
                  break;
              }
              break;
            case 7: sensitivityRatio = limitIncrFloat(sensitivityRatio, -0.1, 0.0, 10);
              break;
            case 8: colorStart = limitIncrInt(colorStart, -10, 0, 255);
              break;
          }
        }
        break;
      case BUTT_LEFT:
        if (settings_mode) {
          // ВЛЕВО общие настройки
          brightness = limitIncrInt(brightness, -20, 0, 255);
          FastLED.setBrightness(brightness);
        } else {
          switch (currentMode) {
            case 0:
            case 1: smoothRatio = limitIncrFloat(smoothRatio, -0.05, 0.05, 1);
              break;
            case 2:
            case 3:
            case 4: freqSmoothRatio = limitIncrFloat(freqSmoothRatio, -0.05, 0.05, 1);
              break;
            case 5: strobeFluency = limitIncrInt(strobeFluency, -20, 0, 255);
              break;
            case 6:
              switch (light_mode) {
                case 0: lightColor = limitIncrInt(lightColor, -10, 0, 255);
                  break;
                case 1: colorChangingSpeed = limitIncrInt(colorChangingSpeed, -10, 0, 255);
                  break;
                case 2: rainbowPeriod = limitIncrInt(rainbowPeriod, -1, -20, 20);
                  break;
              }
              break;
            case 7: runFreqSpeed = limitIncrInt(runFreqSpeed, -10, 1, 255);
              break;
            case 8: colorStep = limitIncrInt(colorStep, -1, 1, 255);
              break;
          }
        }
        break;
      case BUTT_RIGHT:
        if (settings_mode) {
          // ВПРАВО общие настройки
          brightness = limitIncrInt(brightness, 20, 0, 255);
          FastLED.setBrightness(brightness);
        } else {
          switch (currentMode) {
            case 0:
            case 1: smoothRatio = limitIncrFloat(smoothRatio, 0.05, 0.05, 1);
              break;
            case 2:
            case 3:
            case 4: freqSmoothRatio = limitIncrFloat(freqSmoothRatio, 0.05, 0.05, 1);
              break;
            case 5: strobeFluency = limitIncrInt(strobeFluency, 20, 0, 255);
              break;
            case 6:
              switch (light_mode) {
                case 0: lightColor = limitIncrInt(lightColor, 10, 0, 255);
                  break;
                case 1: colorChangingSpeed = limitIncrInt(colorChangingSpeed, 10, 0, 255);
                  break;
                case 2: rainbowPeriod = limitIncrInt(rainbowPeriod, 1, -20, 20);
                  break;
              }
              break;
            case 7: runFreqSpeed = limitIncrInt(runFreqSpeed, 10, 1, 255);
              break;
            case 8: colorStep = limitIncrInt(colorStep, 1, 1, 255);
              break;
          }
        }
        break;
      default: eeprom_flag = false;   // если не распознали кнопку, не обновляем настройки!
        break;
    }
    ir_flag = false;
  }
}

void autoLowPass() {
  // для режима VU
  delay(10);                                // ждём инициализации АЦП
  int thisMax = 0;                          // максимум
  int thisLevel;
  for (byte i = 0; i < 200; i++) {
    thisLevel = analogRead(channelR);        // делаем 200 измерений
    if (thisLevel > thisMax)                // ищем максимумы
      thisMax = thisLevel;                  // запоминаем
    delay(4);                               // ждём 4мс
  }
  lowFilter = thisMax + additionalFilter;        // нижний порог как максимум тишины + некая величина

  // для режима спектра
  thisMax = 0;
  for (byte i = 0; i < 100; i++) {          // делаем 100 измерений
    analyzeAudio();                         // разбить в спектр
    for (byte j = 2; j < 32; j++) {         // первые 2 канала - хлам
      thisLevel = fht_log_out[j];
      if (thisLevel > thisMax)              // ищем максимумы
        thisMax = thisLevel;                // запоминаем
    }
    delay(4);                               // ждём 4мс
  }
  lowFilter2 = thisMax + additionalFreqFilter;  // нижний порог как максимум тишины

  if (saveLowFilter && !autoLowFilter) {
    EEPROM.updateInt(70, lowFilter);
    EEPROM.updateInt(72, lowFilter2);
  }
}

void analyzeAudio() {
  for (int i = 0 ; i < FHT_N ; i++) {
    int sample = analogRead(channelRFreq);
    fht_input[i] = sample; // put real data into bins
  }
  fht_window();  // window the data for better frequency response
  fht_reorder(); // reorder the data before doing the fht
  fht_run();     // process the data in the fht
  fht_mag_log(); // take the output of the fht
}

/*
 * Parse pattern:
 *   Data
 *     [0] - 0. Сhange mode, 1. Change Light Color, 2. Light Mode, 3. Brightness, 4. Empty Brightness, 5. Smooth(1,2 mode),
 *           6. Smooth(3..5 mode), 7. Strobe Smooth, 8. Strobe Period, 9. Rainbow Step(mode 2), a. Rainbow Step(mode 7.3),
 *           b. Color change speed c. Rainbow Period, d. Running Lights speed
 *     [1][2][3] - Data pack 1  (convert to Int [0, 1])
 *     [4][5][6] - Data pack 2  (convert to Int [0, 1])
 *     [7][8][9] - Data pack 3  (convert to Int [0, 1])
 *   Empty symbol: #
 *   Additional information: 
 *     Ex. We receive "100" in Data pack 1, that means 100% of deffault, for smoothRatio 100% == 1.0; for strobePeriod == 1000 etc.
 *     1100#50100 - Change Light Color with lightColor = 255 * 1, lightSat = 255 * 0.5, lightBright = 255 * 1;
 */
void bluetoothTick()
{
  BluetoothBuffer = blue.getMessage();
  Serial.write(BluetoothBuffer);
  if (BluetoothBuffer[0] != '#')
    switch (BluetoothBuffer[0])
    {
      Serial.write("Bluetooth !#");
      memoryTimer = millis();
      eeprom_flag = true;
      case '0': // Сhange mode
        subInt = getInt(1, 3) * 100;
        if (subInt <= numOfModes)
          currentMode = subInt;
        Serial.write("Mode change!");
        break;
      case '1': // Change Light Color
        lightColor = getInt(1, 3) * 255;
        lightSat = getInt(4, 6) * 255;
        lightBright = getInt(7, 9) * 255;
        break;
      case '2': // Light Mode
        light_mode = getInt(1, 3) * 100;
        break;
      case '3': // Общая яркость
        subInt = getInt(1, 3);
        if ( subInt <= 1 ) brightness = subInt * 255;
        FastLED.setBrightness(brightness);
        break;
      case '4': // Яркость "не горящих"
        subInt = getInt(1, 3);
        if ( subInt <= 1 ) backgroundBright = subInt * 255;
        break;
      case '5': // Плавность(1,2)
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        smoothRatio = subInt;
        break;
      case '6': // Плавность(3..5)
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        freqSmoothRatio = subInt;
        break;
      case '7': // Плавность вспышек
        subInt = getInt(1, 3);
        if ( subInt < 0.05 )
          subInt = 0.05;
        strobeFluency = subInt;
        break;
      case '8': // Частота вспышек
        subInt = getInt(1, 3) * 1000;
        if ( subInt < 20 )
          subInt = 20;
        strobePeriod = subInt;
        break;
      case '9': // Шаг радуги 1
        subInt = getInt(1, 3) * 20;
        if ( subInt < 0.5 )
          subInt = 0.5;
        rainbowDist = subInt;
        break;
      case 'a': // Шаг радуги 2
        subInt = getInt(1, 3) * 10;
        if ( subInt < 0.5 )
          subInt = 0.5;
        rainbowDist_2 = subInt;
        break;
      case 'b': // Скорость изменения цвета
        subInt = getInt(1, 3) * 255;
        if ( subInt < 10 )
          subInt = 10;
        colorChangingSpeed = subInt;
        break;
      case 'c': // Период радуги
        subInt = getInt(1, 3) * 20;
        if ( subInt < 1 )
          subInt = 1;
        rainbowPeriod = subInt;
        break;
      case 'd': // Скорость бегущих частот
        subInt = getInt(1, 3) * 255;
        if ( subInt < 10 )
          subInt = 10;
        runFreqSpeed = subInt;
        break;
      default:
        eeprom_flag = true;
        break;
    }
  BluetoothBuffer[0] = '#';
}
 
double getInt(short startPos, const int endPos)
{
  int intBuffer[3] = {0, 0, 0};
  int count = 0;
  for (startPos; startPos <= endPos; startPos++)
  {
    if (BluetoothBuffer[startPos] != '#')
      intBuffer[count] = BluetoothBuffer[startPos] - '0';
    count++;
  }
  return intBuffer[0] + intBuffer[1] / 10 + intBuffer[2] / 100; // Возвращаем числовое значение
}

void fullLowPass()
{
  digitalWrite(13, HIGH);   // включить светодиод 13 пин
  FastLED.setBrightness(0); // погасить ленту
  FastLED.clear();          // очистить массив пикселей
  FastLED.show();           // отправить значения на ленту
  delay(500);               // подождать чутка
  autoLowPass();            // измерить шумы
  delay(500);               // подождать
  FastLED.setBrightness(brightness);  // вернуть яркость
  digitalWrite(13, LOW);    // выключить светодиод
}
void updateEEPROM() {
  EEPROM.updateByte(1, currentMode);
  EEPROM.updateByte(2, freq_strobe_mode);
  EEPROM.updateByte(3, light_mode);
  EEPROM.updateInt(4, rainbowDist);
  EEPROM.updateFloat(8, sensitivityRatio);
  EEPROM.updateInt(12, strobePeriod);
  EEPROM.updateInt(16, lightSat);
  EEPROM.updateFloat(20, rainbowDist_2);
  EEPROM.updateInt(24, colorStart);
  EEPROM.updateFloat(28, smoothRatio);
  EEPROM.updateFloat(32, freqSmoothRatio);
  EEPROM.updateInt(36, strobeFluency);
  EEPROM.updateInt(40, lightColor);
  EEPROM.updateInt(44, colorChangingSpeed);
  EEPROM.updateInt(48, rainbowPeriod);
  EEPROM.updateInt(52, runFreqSpeed);
  EEPROM.updateInt(56, colorStep);
  EEPROM.updateInt(60, backgroundBright);
}
void readEEPROM() 
{
  currentMode = EEPROM.readByte(1);
  freq_strobe_mode = EEPROM.readByte(2);
  light_mode = EEPROM.readByte(3);
  rainbowDist = EEPROM.readInt(4);
  sensitivityRatio = EEPROM.readFloat(8);
  strobePeriod = EEPROM.readInt(12);
  lightSat = EEPROM.readInt(16);
  rainbowDist_2 = EEPROM.readFloat(20);
  colorStart = EEPROM.readInt(24);
  smoothRatio = EEPROM.readFloat(28);
  freqSmoothRatio = EEPROM.readFloat(32);
  strobeFluency = EEPROM.readInt(36);
  lightColor = EEPROM.readInt(40);
  colorChangingSpeed = EEPROM.readInt(44);
  rainbowPeriod = EEPROM.readInt(48);
  runFreqSpeed = EEPROM.readInt(52);
  colorStep = EEPROM.readInt(56);
  backgroundBright = EEPROM.readInt(60);
}
void eepromTick()
{
  if (eeprom_flag)
    if (millis() - memoryTimer > 10000) 
    {
      eeprom_flag = false;
      memoryTimer = millis();
      updateEEPROM();
    }
}
