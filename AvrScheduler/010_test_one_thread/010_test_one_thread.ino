#include"AvrScheduler.h"

volatile bool ready;
void greetings(){
    ready = true;
}

int count = 0;

void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(9600);
  init_tasks();
  ready = false;
  add_callback_task(1, 500000, greetings);
  wake_tasks();
}

void loop() {
  if (ready){
    Serial.print("Time: ");
    Serial.print(millis());
    Serial.println(". Hello!");  
    ready = false;
  }
  // delay(100);
  // Serial.print("TCNT1: ");
  // Serial.println(TCNT1);
  // Serial.print("OCR1A: ");
  // Serial.println(OCR1A);
  // Serial.print("wtime: ");
  // Serial.println(next_task_time());
}
