#include"AvrScheduler.h"

#define ServoPin 4

short angle, dir;

byte servo_task;

// These values work well with the servo I have.
// The values must be a little bit beyond the range
// to allow for the variation between individaul servos.

static const short min_angle=2;
static const short max_angle=250;
short angle_step= 1;

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
    set_task_duty(servo_task, angle);
    // Serial.print("angle: ");
    // Serial.println(angle);
}

Period t;
Period n;
void setup(){
  // Initialize this first so that tracing can be done.
  //Serial.begin(9600);
  Serial.begin(115200);
  init_tasks();

  dir = angle_step;
  angle = 128;
  servo_task = add_servo_task(1, ServoPin, angle);
  add_callback_task(2, 10000, update_angle);
}

void loop() {
    run_task();
}
