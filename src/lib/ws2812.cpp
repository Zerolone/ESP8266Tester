#include "WS2812.h" 


void ws2812Test(String pin, String num, String color, String bright) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  
  if(color=="1") r=255;
  if(color=="2") g=255;
  if(color=="3") b=255;
  if(color=="4"){r=0;g=0;b=0;  }

  pinMode(pin.toInt(), OUTPUT);

  Adafruit_NeoPixel strip(num.toInt(), pin.toInt(), NEO_GRB + NEO_KHZ800);

  strip.begin();
  strip.setBrightness(bright.toInt());

  for (int i = 0; i < num.toInt(); i++) {
    strip.setPixelColor(i, strip.Color(r, g, b)); // Red color
  }
  strip.show();
} 


