#include"AvrScheduler.h"

#define    LeftWheelPwm    5
#define    LeftWheelPin2   6
#define    LeftWheelPin1   7

Duty power, dir;

TaskIndex motor_task, power_task;

static const short min_power=1;
static const short max_power=30;
Duty power_step=1;
bool logs_reported = false;

void update_power(){
    if (power>= max_power){
        power = max_power;
        dir = -power_step;
    }
    if (power <= min_power){
        power = min_power;
        dir = power_step;
    }
    power += dir;
    set_task_duty(motor_task, power);
    // Serial.print("power: ");
    // Serial.println(power);
}

Period t;
Period n;

void setup(){
  // Initialize this first so that tracing can be done.
  //Serial.begin(9600);
  Serial.begin(115200);
  dir = power_step;
  power = (min_power+max_power)/2;
  init_tasks();
  
  pinMode(LeftWheelPin1, OUTPUT);
  pinMode(LeftWheelPin2, OUTPUT);
  digitalWrite(LeftWheelPin1, 1);
  digitalWrite(LeftWheelPin2, 0);
  motor_task = add_motor_task(1, LeftWheelPwm, power);
  set_task_wtime(motor_task, 300);
  power_task = add_callback_task(2, 50000, update_power);
  set_task_wtime(power_task, 450);
  wake_tasks();
}

void loop() {
  #ifndef TRACE_SCHEDULER
    delay(1000);
  #endif
  if (!logs_reported){
    #ifdef TRACE_SCHEDULER
      for(int i=0; i<15; i++){
        run_next_task();
      }
    #endif
    report_logs();
    logs_reported = true;
  }
}
