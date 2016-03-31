#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>     // wifi access
extern "C" {
#include "user_interface.h"
}
#include <WiFiManager.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char clientName[] = "OrbLight";
#define PIN 5
#define NUM_PIXELS 16
#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define WHITE 0x7F7F7F

const int LED_PIN = 5; // Thing's onboard, green LED

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

WiFiServer server(80);


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//
// Blend two colors together based on the ratio. ratio of 0.0 will be 100% color a and
// ratio of 1.0 will be 100% color b.
uint32_t blend (uint32_t ina, uint32_t inb, double ratio)
{
  uint32_t r = (((ina >> 16) & 0xff) * (1.0-ratio)) + (((inb >> 16) & 0xff) * ratio);
  uint32_t g = (((ina >> 8) & 0xff) * (1.0-ratio)) + (((inb >> 8) & 0xff) * ratio);
  uint32_t b = (((ina >> 0) & 0xff) * (1.0-ratio)) + (((inb >> 0) & 0xff) * ratio);
  return ((r << 16) | (g << 8) | b);
}

struct colorValue {
  uint8_t value;
  uint32_t color;
};

uint32_t getColorForValue(int value, const struct colorValue *colorValues, int numValues)
{
  int foundIndex;
  for (foundIndex = 0; foundIndex < numValues; foundIndex++)
    if (value < pgm_read_byte(&(colorValues[foundIndex].value)))
      break;
  double ratio = 0;
  if (foundIndex == 0)
    return pgm_read_dword(&(colorValues[0].color));
  else if (foundIndex == numValues)
    return pgm_read_dword(&(colorValues[numValues-1].color));
  else
  {
    uint32_t prevColor = pgm_read_dword(&(colorValues[foundIndex-1].color));
    uint32_t nextColor = pgm_read_dword(&(colorValues[foundIndex].color));
    uint8_t nextValue = pgm_read_byte(&(colorValues[foundIndex].value));
    uint8_t prevValue = pgm_read_byte(&(colorValues[foundIndex-1].value));
    double ratio = ((double)value - prevValue) / (nextValue - prevValue);
    return blend (prevColor, nextColor, ratio);
  }
  return 0;
}

// Functions list
enum functions
{
  TEMPERATURE,
  COLOR,
  PULSE,
  COLOR_CHASE,
  RAINBOW
};

int currentFunction = TEMPERATURE;   // default
uint32_t COLOR_parameter = 0xffffff; // parameter for color function
uint32_t COLOR_parameter2 = 0xffffff; // parameter for color function

float currentTemperature = 0;

void connectWiFi()
{
  strip.begin();
  strip.setPixelColor(0, 0xff0000);
  strip.show(); // Initialize all pixels to 'off'
  wifi_station_set_hostname((char*)clientName);
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  wifiManager.autoConnect("OrbLightAP");

  strip.setPixelColor(0, 0xffff00);
  strip.show(); // Initialize all pixels to 'off'
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectServer()
{
  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  
}


void pollServer()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
   
  // Wait until the client sends some data
  int counter = 0;
  while(!client.available()){
    delay(10);
    if (counter++ > 100) // timeout
    {
      Serial.println ("client timeout");
      return;
    }
  }
   
  // Read the first line of the request
  String request = client.readStringUntil('\r');
//  Serial.println(request);
  int startIndex = request.indexOf("?");
  if (startIndex >= 0)
  {
    String params = request.substring(startIndex+1, request.indexOf(" ", startIndex));
    startIndex = 0;
    while (1)
    {
      int equalIndex = params.indexOf("=", startIndex);
      int andIndex = params.indexOf("&", startIndex);
      if (equalIndex >= 0)
      {
        String key = params.substring(startIndex, equalIndex);
        String value = (andIndex >= 0) ? params.substring(equalIndex+1, andIndex) : params.substring(equalIndex+1);
        if (key == "mode")
        {
          if (value == "temperature")
          {
            Serial.println("Temperature function");
            currentFunction = TEMPERATURE;
         } else if (value == "color") {
            Serial.println("Color function");
            currentFunction = COLOR;          
         } else if (value == "pulse") {
            Serial.println("Pulse color function");
            currentFunction = PULSE;          
         } else if (value == "color_chase") {
            Serial.println("Color chase function");
            currentFunction = COLOR_CHASE;          
         } else if (value == "rainbow") {
            Serial.println("Rainbow function");
            currentFunction = RAINBOW;          
         }
        }
        else if (key == "color1")
        {
          COLOR_parameter = strtoul(value.c_str(), 0, 0);
          Serial.println("Color parameter");
         Serial.println(value);  
        }
        else if (key == "color2")
        {
          COLOR_parameter2 = strtoul(value.c_str(), 0, 0);
          Serial.println("Color parameter 2");
         Serial.println(value);  
        }
      }
      if (equalIndex == -1 || andIndex == -1)
        break;
      startIndex = andIndex+1;
    }
  }
  client.flush(); 
 
  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<a href=\"/?mode=temperature\">Display temperature: ");
  client.println(currentTemperature);
  client.println("<br></a>");
  client.println("<a href=\"/?mode=color&color1=0xff0000\">Display color RED<br></a>");
  client.println("<a href=\"/?mode=color&color1=0x00ff00\">Display color GREEN<br></a>");
  client.println("<a href=\"/?mode=color&color1=0x0000ff\">Display color BLUE<br></a>");
  client.println("<a href=\"/?mode=pulse&color1=0xff0000\">Pulse color RED<br></a>");
  client.println("<a href=\"/?mode=pulse&color1=0x00ff00\">Pulse color GREEN<br></a>");
  client.println("<a href=\"/?mode=pulse&color1=0x0000ff\">Pulse color BLUE<br></a>");
  client.println("<a href=\"/?mode=color_chase&color1=0xff0000&color2=0x00ff00\">Color chase RED/GREEN<br></a>");
  client.println("<a href=\"/?mode=rainbow\">Rainbow mode<br></a>");
  client.println("</html>");
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Temperature display function
//
// The temperature -> color mapping table is going into program space to save ram
#define NUM_TEMPS 8
static const PROGMEM struct colorValue colorsForTemps[NUM_TEMPS] =
{
  { 30, 0xff00ff }, // purple
  { 40, 0x3f00ff }, // blue purple
  { 50, 0x0000ff }, // blue
  { 60, 0x00ffd0 }, // turquoise
  { 70, 0x00ff00 }, // green
  { 80, 0xfdff00 }, // yellow
  { 90, 0xFFa000 }, // orange
  { 100, 0xff0000 } // red
};

void getTime()
{
  const int TIME_CHECK_FREQUENCY = 600000; // 10 minutes
  static unsigned long lastPoll = 0;
  static const char server[] = "www.timeapi.org";
  static const char location[] = "GET /utc/now/ HTTP/1.1";
  WiFiClient client;
      
  // We only want to grab the temperature every 10 minutes
  unsigned long currentTime = millis();
  if (lastPoll == 0 || (currentTime - lastPoll) > TIME_CHECK_FREQUENCY)
  {
    lastPoll = currentTime;
    
  //connect to the server
  Serial.print("connecting to ");
  Serial.println(server);

  //port 80 is typical of a www page
  if (client.connect(server, 80)) {
    Serial.println(location);
    client.println(location);
    client.println("Connection: close");
    client.println();

    String response;
    while (client.available())
      response += (char)client.read();
    Serial.print ("Response = ");
    Serial.println(response);

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
  }
  }
  }
}

void showTemp(){
  const int TEMPERATURE_CHECK_FREQUENCY = 600000; // 10 minutes
  static unsigned long lastPoll = 0;
  static uint32_t currentTemperatureColor = 0;
  static const char server[] = "api.openweathermap.org";
  static const char location[] = "GET /data/2.5/weather?zip=94538,us&APPID=b02ef0450929750e35b63e7591fcdbc2&units=imperial";
  WiFiClient client;

  // We only want to grab the temperature every 10 minutes
  unsigned long currentTime = millis();
  if (lastPoll == 0 || (currentTime - lastPoll) > TEMPERATURE_CHECK_FREQUENCY)
  {
    lastPoll = currentTime;
    
    //connect to the server
    Serial.println(server);

    //port 80 is typical of a www page
    if (client.connect(server, 80)) {
      Serial.println(location);
      client.println(location);
      client.println();

      String response;
      while (client.available())
        response += (char)client.read();

      //Serial.println(response);
  
      int indexOfTemp = response.indexOf("\"temp\":");
      if (indexOfTemp >= 0)
      {
        int indexOfCommaAfterTemp = (indexOfTemp >= 0) ? response.indexOf(",", indexOfTemp) : -1;
        String tempString = (indexOfCommaAfterTemp >= 0) ? response.substring (indexOfTemp+7, indexOfCommaAfterTemp) : "";
        float tempKelvin = (tempString.length() > 0) ? atof(tempString.c_str()) : 0;
        currentTemperature = (tempKelvin > 0) ? (tempKelvin * 9 / 5 - 459.67) : -1;
//        Serial.println(temperature);
    
        currentTemperatureColor = getColorForValue(currentTemperature, colorsForTemps, NUM_TEMPS);
      }
    }
  }     
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++)
    strip.setPixelColor(pixel, currentTemperatureColor);
  strip.show();
}

////////////////////////////////////////////////////////////////////////////
// Solid color function
void showColor()
{
  for (int pixel = 0; pixel < NUM_PIXELS; pixel++)
    strip.setPixelColor(pixel, COLOR_parameter);
  strip.show();  
}

////////////////////////////////////////////////////////////////////////////
// Pulse color function
void pulseColor()
{
  for (int i = 0; i < 510; i++)
  {
    if (i < 256)
      strip.setBrightness(255-i);
    else
      strip.setBrightness(i-255);
    showColor();
    pollServer();
    if (currentFunction != PULSE)
    {
      strip.setBrightness(255);
      break;
    }
    delay(10);
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////
// Color chase function
void colorChase()
{
  for (int i1 = 0; i1 < NUM_PIXELS; i1++)
  {
    for (int i2 = 0; i2 < NUM_PIXELS; i2++)
      strip.setPixelColor((i2 + i1) % NUM_PIXELS, (i2 < NUM_PIXELS/2) ? COLOR_parameter : COLOR_parameter2);
    strip.show();
    delay(50);
    pollServer();
    if (currentFunction != COLOR_CHASE)
      break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Color chase function
void rainbow()
{
  for (int i1 = 0; i1 < 256; i1++)
  {
    for (int i2 = 0; i2 < NUM_PIXELS; i2++)
      strip.setPixelColor (i2, Wheel(i1));
    strip.show();
    delay(50);
    pollServer();
    if (currentFunction != RAINBOW)
      break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Setup and main loop
//
void setup() {
  Serial.begin(9600);
  connectWiFi();
  connectServer();

  strip.setBrightness (0xff);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

}

void loop(){

  pollServer();
//  getTime();
  switch (currentFunction)
  {
    case COLOR:       showColor(); break;
    case TEMPERATURE: showTemp(); break;
    case PULSE:       pulseColor(); break;
    case COLOR_CHASE: colorChase(); break;
    case RAINBOW:     rainbow(); break;
  }
  delay(10); // give the processor time to do its thing
}








