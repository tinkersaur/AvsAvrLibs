#include"AvrScheduler.h"

volatile bool cond_a, cond_b, cond_c;

void greetingsA(){
  cond_a = true;
}

void greetingsB(){
  cond_b= true;
}

void greetingsC(){
  cond_c = true;
}

void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(115200);
  init_tasks();
  add_callback_task(1, 2500000, greetingsA);
  add_callback_task(2, 3300000, greetingsB);
  add_callback_task(3, 5000000, greetingsC);
  cond_a = false;
  cond_b = false;
  cond_c = false;
  wake_tasks();
}

void loop() {
  if (cond_a){ 
    cond_a = false;
    Serial.print("Time: ");
    Serial.print(quick_millis());
    Serial.println(". A");  
  }
  if (cond_b){ 
    cond_b = false;
    Serial.print("Time: ");
    Serial.print(quick_millis());
    Serial.println(". B");  
  }
  if (cond_c){ 
    cond_c = false;
    Serial.print("Time: ");
    Serial.print(quick_millis());
    Serial.println(". C");  
  }
}
