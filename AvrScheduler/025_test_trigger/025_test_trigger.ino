#include"AvrScheduler.h"

volatile bool cond_a, cond_b, cond_c;
TaskIndex task_a, task_b, task_c;
bool logs_reported=false;

void setup(){
  // Initialize this first so that tracing can be done.
  Serial.begin(115200);
  init_tasks();
  task_c = add_trigger_task(3, 500000, &cond_c);
  task_b = add_trigger_task(2, 330000, &cond_b);
  task_a = add_trigger_task(1, 250000, &cond_a);
  set_task_wtime(task_a, 300000);
  set_task_wtime(task_b, 300200);
  set_task_wtime(task_c, 300400);
  cond_a = false;
  cond_b = false;
  cond_c = false;
  //report_tasks(); 
  wake_tasks();
}

void loop() {
  #ifdef TRACE_SCHEDULER
    if (!logs_reported){
      TCNT1 = 75;
      for(int i=0; i<5; i++){
        // run_next_task();
        simulate_compare_interrupt();
        TCNT1 += 50;
      }
      report_logs();
      logs_reported = true;
    }
  #else 
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
    // if (!logs_reported && logs_full()){
    //   report_logs();
    //   logs_reported = true;
    // }
  #endif
}
