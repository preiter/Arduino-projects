#include <avr/pgmspace.h>
#define FHT_N 256
#define LIN_OUT 1
#include <FHT.h>
#include <Adafruit_NeoPixel.h>
#include <LED_animations.h>
unsigned long millis(void);

#define PIN 6 // digital pin for programming neopixels
#define NUM_PIXELS 48 // this is the size of my neopixel strip 
#define GROUP_SIZE 6  // num pixels in each of the sections
//#define NUM_PIXELS 64 // this is the size of my neopixel strip 
//#define GROUP_SIZE 8  // num pixels in each of the sections
#define BUTTON 3      // Pushbutton connected to gpio 3
#define ROTARY_A 2    // Rotary encoder connected to gpio 2 and 5
#define ROTARY_B 5           

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
LED_animations animations (strip, 50, GROUP_SIZE);

#define ADC_CHANNEL 0                // microphone input
volatile uint32_t samplePos = 0;     // Buffer position counter

static const uint8_t PROGMEM
  // This is low-level noise that's subtracted from each FHT output column
  // This was experimentally determined in a quiet room.
  noise[128]={ 
    50, 12, 10, 8, 7, 6, 6, 5, // 0
    5, 5, 4, 4, 4, 4, 4, 4,    // 8
    4, 4, 4, 4, 4, 4, 4, 4,    // 16
    4, 4, 4, 4, 4, 4, 4, 4,    // 24
    4, 4, 4, 4, 4, 4, 4, 4,    // 32
    3, 3, 3, 4, 3, 3, 3, 3,    // 40
    3, 3, 3, 3, 3, 3, 3, 3,    // 48    
    3, 3, 3, 3, 3, 3, 3, 3,    // 56
    3, 3, 3, 3, 3, 3, 3, 3,    // 64
    3, 3, 3, 3, 3, 3, 3, 3,    // 72
    3, 3, 3, 3, 3, 3, 3, 3,    // 80
    3, 3, 3, 3, 3, 3, 3, 3,    // 88
    3, 3, 3, 3, 3, 3, 3, 3,    // 96
    3, 3, 3, 3, 3, 3, 3, 3,    // 104
    3, 3, 3, 3, 3, 3, 3, 3,    // 112
    3, 3, 3, 3, 3, 3, 3, 3     // 120
};

// the 128 output buckets of the fht transform get compressed down 
// to fewer slots for display. This array is the starting output 
// bucket for each display slot. You use more output buckets per
// display slot as you go up.
static const uint8_t PROGMEM
  slots[] = {
    1, 2, 3, 6, 10, 18, 34, 66, 127
  } ;

#define THRESHOLD 1 

void setup() {
  strip.setBrightness(255);
  // set up the input and interrupt for the mode switch button press
  pinMode (BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), ISR2, FALLING);
  pinMode (ROTARY_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ROTARY_A), ISR3, CHANGE);
  pinMode (ROTARY_B, INPUT_PULLUP);

// The prescaler settings determine the frequency of audio sampling. We can sample higher
// frequencies with a lower prescaler value, but it will also raise the lowest frequency that
// we can sample. With this setup, I seem to be getting around 300Hz-9.6KHz response. There is
// some aliasing going on, meaning that frequencies > 9.6KHz will show up in lower frequency
// response.
  // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion 
  ADMUX  = _BV(6) | ADC_CHANNEL; // Channel sel, right-adj, use AREF pin
  ADCSRA = _BV(ADEN)  | // ADC enable
           _BV(ADSC)  | // ADC start
           _BV(ADATE) | // Auto trigger
           _BV(ADIE)  | // Interrupt enable
           // select the prescaler value. Note that the max frequency our FFT will
           // display is half the sample rate.
//           _BV(ADPS2) | _BV(ADPS0); // 32:1 / 13 = 38,460 Hz
           _BV(ADPS2) | _BV(ADPS1); // 64:1 / 13 = 19,230 Hz
//           _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
  ADCSRB = 0;                // Free run mode, no high MUX bit
  DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin
  // Neopixels setup
  strip.begin(); // Initialize all pixels to 'off'
  strip.show();  // write to neopixels
  Serial.begin(9600);  // set up Serial library at 9600 bps for debugging purposes
  Serial.println("begin final");
}

// Animation modes that you can cycle through
enum Mode {
  ModeSpectrumAnalyzer,
  ModeVUMeter,
  ModeRainbowChase,
  ModeColorWheel,
  ModeRainbow2,
  ModeWipeRed,
  ModeWipeGreen,
  ModeWipeBlue,
  ModeChaseRed,
  ModeChaseGreen,
  ModeChaseBlue,
  ModeCount
};
volatile int CurrentMode = ModeSpectrumAnalyzer;
volatile int brightness = 0xff;
volatile unsigned long lastInterrupt = 0;

void loop() {
  strip.setBrightness(brightness);
  switch (CurrentMode)
  {
      case ModeSpectrumAnalyzer: SpectrumAnalyzerMode(); break;
      case ModeVUMeter: VUMeterMode(); break;
      case ModeColorWheel: animations.Rainbow(80); break;  // display about 1/3 of the spectrum at once
      case ModeRainbow2: animations.Rainbow(256); break; // display whole spectrum at once
      case ModeWipeRed: animations.ColorWipe(0xff0000, 200); break;
      case ModeWipeGreen: animations.ColorWipe(0x00ff00, 200); break;
      case ModeWipeBlue: animations.ColorWipe(0x0000ff, 200); break;
      case ModeChaseRed: animations.TheaterChase(0xff0000, 200); break;
      case ModeChaseGreen: animations.TheaterChase(0x00ff00, 200); break;
      case ModeChaseBlue: animations.TheaterChase(0x0000ff, 200); break;
      case ModeRainbowChase: animations.TheaterChaseRainbow(2000); break; 
  } 
}

//
// Run a FHT algorithm and massage the results into colors to display on LEDs
void SpectrumAnalyzerMode()
{
  while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish
  // Do the FHT algorithm
  fht_window();
  fht_reorder();
  fht_run();
  fht_mag_lin();
  //
  // remove low level noise, stops the LEDs from blinking when there is no sound
  // this is frequency dependant and experimentally determined.
  for (int i = 0; i < 128; i++)
  {
      int noiseVal = pgm_read_byte(&noise[i]) << 6;
      fht_lin_out[i] = (fht_lin_out[i] > noiseVal) ? fht_lin_out[i]-noiseVal : 0;
  }
  samplePos = 0;                   // Reset sample counter
  ADCSRA |= _BV(ADIE);             // Resume sampling interrupt
  
  // Combine the 128 frequency output slots into a smaller number of output
  // buckets, since we only have 8 lights. The FHT output is linear. Each
  // bucket accounts for the same range of frequencies. This is non-ideal for
  // audio because we percieve frequencies with a log2 response. So allocate
  // more FHT output buckets to display slots as you go up the frequency range.
  for (int i = 0; i < 8; i++)
  {
      int value = 0;
      int start = pgm_read_byte(&slots[i]);
      int end = pgm_read_byte(&slots[i+1]);
      for (int i2 = start; i2 < end; i2++)
        value += (fht_lin_out[i2] >> 6);
      if (value < THRESHOLD)
        value = 0;

      // display the output on the LED array.
      animations.setPixelColor(i, animations.logWheel(value));
  }
  animations.show(); // write to the neopixel strip
}

//
// Technically this isn't a real VU meter. I'm just summing the low frequency bins of the FHT output
// to get the bass intensity. This bounces up and down nicely with the beat if you are listening to
// dancy music.
void VUMeterMode()
{
  const float FUDGE_FACTOR = 1.25; // adjust so that we get a good response going up to the top bar
  while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish
  // Do the FHT algorithm
  fht_window();
  fht_reorder();
  fht_run();
  fht_mag_lin();
  //
  // remove low level noise, stops the LEDs from blinking when there is no sound
  for (int i = 0; i < 128; i++)
  {
    int noiseVal = pgm_read_byte(&noise[i]) << 6;
    fht_lin_out[i] = (fht_lin_out[i] > noiseVal) ? fht_lin_out[i]-noiseVal : 0;
  }
  samplePos = 0;                   // Reset sample counter
  ADCSRA |= _BV(ADIE);             // Resume sampling interrupt
  
  int32_t rms_Level = 0;   // not really RMS
  for (int bucket = 0; bucket < 4; bucket++)   // summing the 4 low frequency bins of the FHT output
      rms_Level += (fht_lin_out[bucket] >> 6); // down convert from 16 bit signed values of the FHT
  rms_Level *= FUDGE_FACTOR;                   // typical music never triggerd the max level, so scale it up

  for (uint8_t row = 0; row < 8; row++)
  {
      uint32_t rowColor = (rms_Level > row*8) ? animations.logWheel(row*8) : 0;
      animations.setPixelColor(row, rowColor);
  }
  animations.show();
}

// interrupt service routine. This gets called each time the ADC finishes 1 sample.
ISR(ADC_vect) {                         // Audio-sampling interrupt
    uint32_t sample = ADC - 256;        // ADC values are 0-1024, but with this mic and aref we'll only see half that.
                                        // convert to +/- 256 by subtracting off 256 
    fht_input[samplePos] = sample << 6;  // then left shift it up to a 16 bit signed for the FFT algorithm
                                        // Since the FFT is using integer arithmetic, we want the values as large
                                        // as possible.                                          
    if(++samplePos >= FHT_N) 
        ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}

// another interrupt service routine for the button input
void ISR2() {
  unsigned long m = millis();
  if ((m - lastInterrupt) > 500) // we get multiple interrupts for each button press
  {                               // so only allow one mode switch per second
    CurrentMode = (CurrentMode+1) % ModeCount;
    animations.SetInterruptFlag(); // make the current animation bail out early
    lastInterrupt = m;
    Serial.print ("mode="); 
    Serial.println(CurrentMode);
    }
}


enum {  NOT_LEGAL,  CW,  CCW}; // impossible state, clockwise turn, counter clockwise turn

//
// This is a table of every combination of AB (previous read) -> AB (current read).
// Only certain state transitions are meaningful.
static const uint8_t PROGMEM
legalTransitions[]  = {
  NOT_LEGAL, // 00->00
  CCW,       // 00->01
  CW,        // 00->10
  NOT_LEGAL, // 00->11
  CW,        // 01->00
  NOT_LEGAL, // 01->01
  NOT_LEGAL, // 01->10
  CCW,       // 01->11
  CCW,       // 10->00
  NOT_LEGAL, // 10->01
  NOT_LEGAL, // 10->10
  CW,        // 10->11
  NOT_LEGAL, // 11->00
  CW,        // 11->01
  CCW,       // 11->10
  NOT_LEGAL  // 11->11
};

//
// interrupt service routine for the rotary encoder
// When we get this interrupt, we start reading both the A and the B inputs for the rotary
// encoder until we see a legal state transition. The transisiton should tell us which way
// the encoder was turned.
void ISR3() {
    unsigned long m = millis();
    if ((m - lastInterrupt) < 50) // we get multiple interrupts for each turn of the encoder
      return;                     // so don't process one right after another
    //
    // Read the input pins up to 50 times looking for a legal transition.
    int lastState = -1; // unknown previous state
    for (int i = 0; i < 5000; i++)
    {
        int state = ((digitalRead(ROTARY_A) == HIGH) ? 0x2 : 0) |  ((digitalRead(ROTARY_B) == HIGH) ? 0x1 : 0);
        int transition = lastState << 2 | state;
        if (lastState == -1) // unknown previous state
        {
            lastState = state;
        }
        else
        {
            int transitionState = pgm_read_byte(&legalTransitions[transition]);
            if (transitionState == CW || transitionState == CCW)
            {
                brightness = constrain(brightness + ((transitionState == CW) ? -20 : 20), 0, 255);
                animations.SetInterruptFlag(); // make the current animation bail out early
//                Serial.println(brightness); // debug 
//                Serial.println(transitionState == CW ? "CW" : "CCW");
                lastInterrupt = m;
                break;    
           }
        }
    }
}

