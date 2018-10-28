#include"AvrScheduler.h"

void greetingsA(){
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.println(". A");  
}


void greetingsB(){
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.println(". B");  
}


void greetingsC(){
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.println(". C");  
}
void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(9600);
  init_tasks();
  add_callback_task(1, 250000, greetingsA);
  add_callback_task(2, 330000, greetingsB);
  add_callback_task(3,  50000, greetingsC);
}

void loop() {
  run_task();
}
