#include"AvrScheduler.h"

#define ServoPin 4

short angle, dir;

short angle_step= 1;

byte servo_task;

void update_angle(){
    if (angle>=255){
        angle = 255;
        dir = -angle_step;
    }
    if (angle <=0){
        angle = 0;
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
  Serial.begin(115200);
  angle = 240;
  dir = 5;
  init_tasks();

    // TODO: remove:
  pinMode(ServoPin, OUTPUT); 
  n = 0;
  servo_task = add_servo_task(1, ServoPin, angle);
  //add_callback_task(2, 100000, update_angle);
  // run_task();
  // run_task();
  // run_task();
}
const static Period zero_duty = 625;
const static Period full_duty = 1920;
const static Period servo_period = 20000;
void simple_pwm(){
    Period t = n * servo_period + zero_duty + angle*full_duty/255;
    digitalWrite(ServoPin, 1);
    while(micros()<t);
    t = (n+1) * servo_period;
    digitalWrite(ServoPin, 0);
    while(micros()<t);
    update_angle();
    n++;
}
void loop() {
  //run_task();
  simple_pwm(); 
}
