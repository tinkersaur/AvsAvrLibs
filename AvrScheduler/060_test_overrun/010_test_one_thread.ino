#include"AvrScheduler.h"

void greetings(){
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.println(". Hello!");  
}

void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(9600);
  init_tasks();
  add_callback_task(1, 250000, greetings);
}

void loop() {
  run_task();
}
