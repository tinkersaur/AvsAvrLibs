/**
  The perpose of this test is to test function run_next_task()

  Note that to run this test, setup_timers() must be commented out
  in init_tasks(), and "#define SETUP_ISR" must be also commented out.

  Note that when looking at the output, the time displayed is the
  next time. One has to subtract the period, to obtain the pretend-execute
  time.

  Considering the note above, the output is as expected.
*/

#include"AvrScheduler.h"
#define TRACE
#include"macros.h"

volatile bool cond_a, cond_b, cond_c;

void greetingsA(){
  ENTER();
  cond_a = true;
  TR(next_task_time());
}

void greetingsB(){
  ENTER();
  cond_b= true;
  TR(next_task_time());
}

void greetingsC(){
  ENTER();
  cond_c = true;
  TR(next_task_time());
}

void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(9600);
  init_tasks();
  add_callback_task(1, 250000, greetingsA);
  add_callback_task(2, 330000, greetingsB);
  add_callback_task(3, 500000, greetingsC);
  cond_a = false;
  cond_b = false;
  cond_c = false;
}

void run_next_task();

void loop() {
  run_next_task();
  if (cond_a){ 
    cond_a = false;
  }
  if (cond_b){ 
    cond_b = false;
  }
  if (cond_c){ 
    cond_c = false;
  }
  delay(500);
}
