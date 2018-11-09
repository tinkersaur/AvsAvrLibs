#include"AvrScheduler.h"

void greetings(){
  Serial.print("Time: ");
  Serial.print(millis());
  Serial.println(". Hello!");  
}

int phase;
void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(9600);
  init_tasks();
  add_callback_task(1, 250000, greetings);
  phase=0;
}

void loop() {
    if (timer1_high_count<0xFF){
        if (phase==0){
            phase=1;
            Serial.println("overflow");
        }
    } else
    if (timer1_high_count<0x1FF){
        if (phase==0){
            phase=1;
        }
    }
}
