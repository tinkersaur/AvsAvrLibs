#include"AvrScheduler.h"

#define ServoPin 4

Duty angle, dir;

TaskIndex servo_task, callback_task;

// These values work well with the servo I have.
// The values must be a little bit beyond the range
// to allow for the variation between individaul servos.

static const short min_angle=0;
static const short max_angle=MaxServoDuty;
Duty angle_step= 5;

void update_angle(){
    if (angle>= max_angle){
        angle = max_angle;
        dir = -angle_step;
    }
    if (angle <= min_angle){
        angle = min_angle;
        dir = angle_step;
    }
    angle += dir;
    add_log(quick_micros(), angle);
    set_task_duty(servo_task, angle);
}

Period t;
Period n;
bool logs_reported = false;

void setup(){
  // Initialize this first so that tracing can be done.
  //Serial.begin(9600);
  Serial.begin(115200);
  init_tasks();

  dir = angle_step;
  angle = 0;
  servo_task = add_servo_task(1, ServoPin, angle);
  set_task_wtime(servo_task, 300);
  callback_task = add_callback_task(2, 20000, update_angle);
  set_task_wtime(callback_task, 600);
  wake_tasks();
}

void loop() {
  delay(1000);
  if (!logs_reported){
    report_logs();
    logs_reported = true;
  }
}
