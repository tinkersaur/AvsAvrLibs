#include"AvrScheduler.h"

void greetings(){
  unsigned long t = millis();
  Serial.print("Time: ");
  Serial.print((int)t);
  Serial.println(". Hello!");  
}

void setup(){
  Serial.begin(9600);
  init_tasks();
  add_callback_task(1, 250000, greetings);
}

void loop() {
  //greetings();
  delay(100);
  run_task();
}
