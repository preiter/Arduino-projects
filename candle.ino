// candle for Adafruit NeoPixel
// 8 pixel version
// by Tim Bartlett, December 2013

#include <Adafruit_NeoPixel.h>
#define PIN 6

// color variables: mix RGB (0-255) for desired yellow
int redPx = 0xff;
int grnHigh = 0x64;
int bluePx = 0xa;

uint32_t brightest = 0xff640a;
uint32_t dimmest =   0xaf1a00;

// animation time variables, with recommendations
int burnDepth = 20; //how much green dips below grnHigh for normal burn - 
int flutterDepth = 50; //maximum dip for flutter
int cycleTime = 120; //duration of one dip in milliseconds

// pay no attention to that man behind the curtain
int fDelay;
int fRep;
int flickerDepth;
int burnDelay;
int burnLow;
int flickDelay;
int flickLow;
int flutDelay;
int flutLow;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(64, PIN, NEO_GRB + NEO_KHZ800);

//
// Blend two colors together based on the ratio. ratio of 0.0 will be 100% color a and
// ratio of 1.0 will be 100% color b.
uint32_t blend (uint32_t ina, uint32_t inb, uint32_t ratio)
{
  uint32_t r = ((((ina >> 16) & 0xff) * (256-ratio)) + (((inb >> 16) & 0xff) * ratio)) >> 8;
  uint32_t g = ((((ina >> 8) & 0xff) * (256-ratio)) + (((inb >> 8) & 0xff) * ratio)) >> 8;
  uint32_t b = ((((ina >> 0) & 0xff) * (256-ratio)) + (((inb >> 0) & 0xff) * ratio)) >> 8;
  return ((r << 16) | (g << 8) | b);
}

void setup() {
  strip.setBrightness(0xff);
  flickerDepth = (burnDepth + flutterDepth) / 2.4;
  burnLow = grnHigh - burnDepth;
  burnDelay = (cycleTime / 2) / burnDepth;
  flickLow = grnHigh - flickerDepth;
  flickDelay = (cycleTime / 2) / flickerDepth;
  flutLow = grnHigh - flutterDepth;
  flutDelay = ((cycleTime / 2) / flutterDepth);
  
  strip.begin();
  strip.show();
}

// In loop, call CANDLE STATES, with duration in seconds
// 1. on() = solid yellow
// 2. burn() = candle is burning normally, flickering slightly
// 3. flicker() = candle flickers noticably
// 4. flutter() = the candle needs air!

void loop() {
  burn(10);
  flicker(5);
  burn(8);
 flutter(6);
  burn(3);
  on(10);
  burn(10);
  flicker(10);
}


// basic fire funciton - not called in main loop
void fire(int grnLow) {
  for (int grnPx = grnHigh; grnPx > grnLow; grnPx--) {
    int halfGrn = grnHigh - ((grnHigh - grnPx) / 2);
    uint32_t darkGrn = grnPx - 70;
    darkGrn = constrain(darkGrn, 5, 255);
    uint32_t max = brightest;
    uint32_t min = (0xaf0000) | (darkGrn << 8);
    strip.setPixelColor(0, min);
    strip.setPixelColor(1, min);
    strip.setPixelColor(2, blend (max, min, 168));
    strip.setPixelColor(3, blend (max, min, 84));
    strip.setPixelColor(4, redPx, grnPx, bluePx);
    strip.setPixelColor(5, redPx, grnPx, bluePx);
    strip.setPixelColor(6, redPx, halfGrn, bluePx);
    strip.setPixelColor(7, max);
    strip.show();
    delay(fDelay);
  }  
  for (int grnPx = grnLow; grnPx < grnHigh; grnPx++) {
    int halfGrn = grnHigh - ((grnHigh - grnPx) / 2);
    int darkGrn = grnPx-70;
    darkGrn = constrain(darkGrn, 5, 255);
    strip.setPixelColor(0, redPx-180, darkGrn, 0);
    strip.setPixelColor(1, redPx-180, darkGrn, 0);
    strip.setPixelColor(2, redPx-120, grnPx-50, bluePx-5);
    strip.setPixelColor(3, redPx-60, grnPx-35, bluePx-2);
    strip.setPixelColor(4, redPx, grnPx, bluePx);
    strip.setPixelColor(5, redPx, grnPx, bluePx);
    strip.setPixelColor(6, redPx, halfGrn, bluePx);
    strip.setPixelColor(7, redPx, grnHigh, bluePx);
    strip.show();
    delay(fDelay);
  }
}

// fire animation
void on(int f) {
  fRep = f * 1000;
  int grnPx = grnHigh - 6;
    strip.setPixelColor(0, redPx-180, grnPx-70, 0);
    strip.setPixelColor(1, redPx-180, grnPx-70, 0);
    strip.setPixelColor(2, redPx-120, grnPx-50, bluePx-5);
    strip.setPixelColor(3, redPx-60, grnPx-35, bluePx-2);
    strip.setPixelColor(4, redPx, grnPx, bluePx);
    strip.setPixelColor(5, redPx, grnPx, bluePx);
    strip.setPixelColor(6, redPx, grnPx, bluePx);
    strip.setPixelColor(7, redPx, grnHigh, bluePx);
  strip.show();
  delay(fRep);
}

void burn(int f) {
  fRep = f * 8;
  fDelay = burnDelay;
  for (int var = 0; var < fRep; var++) {
    fire(burnLow);
  }  
}

void flicker(int f) {
  fRep = f * 8;
  fDelay = burnDelay;
  fire(burnLow);
  fDelay = flickDelay;
  for (int var = 0; var < fRep; var++) {
    fire(flickLow);
  }
  fDelay = burnDelay;
  fire(burnLow);
  fire(burnLow);
  fire(burnLow);
}

void flutter(int f) {
  fRep = f * 8;  
  fDelay = burnDelay;
  fire(burnLow);
  fDelay = flickDelay;
  fire(flickLow);
  fDelay = flutDelay;
  for (int var = 0; var < fRep; var++) {
    fire(flutLow);
  }
  fDelay = flickDelay;
  fire(flickLow);
  fire(flickLow);
  fDelay = burnDelay;
  fire(burnLow);
  fire(burnLow);
}
