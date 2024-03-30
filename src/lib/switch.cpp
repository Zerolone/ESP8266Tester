#include "switch.h" 

void switchTest(String pin, String on) {
  pinMode(pin.toInt(), OUTPUT);

  if (on == "0"){
    digitalWrite(pin.toInt(), LOW);
  }else{
    digitalWrite(pin.toInt(), HIGH);
  }
} 


