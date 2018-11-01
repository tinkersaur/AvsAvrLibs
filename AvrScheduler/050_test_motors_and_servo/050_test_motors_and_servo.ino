#include"AvrScheduler.h"

#define    LeftWheelPwm    5
#define    LeftWheelPin2   6
#define    LeftWheelPin1   7

#define    RightWheelPin2   8
#define    RightWheelPin1   9
#define    RightWheelPwm   10

short left_power, right_power, left_dir, right_dir;


byte left_motor_task, right_motor_task;

static const short min_power=0;
static const short max_power=128;
short power_step= 1;

void update_left_power(){
    if (left_power>= max_power){
        left_power = max_power;
        left_dir = -power_step;
    }
    if (left_power <= min_power){
        left_power = min_power;
        left_dir = power_step;
    }
    left_power += left_dir;
    set_task_duty(left_motor_task, left_power);
}

void update_right_power(){
    if (right_power>= max_power){
        right_power = max_power;
        right_dir = -power_step;
    }
    if (right_power <= min_power){
        right_power = min_power;
        right_dir = power_step;
    }
    right_power += right_dir;
    set_task_duty(right_motor_task, right_power);
}

Period t;
Period n;

void setup(){
  // Initialize this first so that tracing can be done.
  //Serial.begin(9600);
  Serial.begin(115200);
  init_tasks();
  
  left_dir = power_step;
  left_power = 200;
  pinMode(LeftWheelPin1, OUTPUT);
  pinMode(LeftWheelPin2, OUTPUT);
  digitalWrite(LeftWheelPin1, 1);
  digitalWrite(LeftWheelPin2, 0);
  left_motor_task = add_motor_task(1, LeftWheelPwm, left_power);
  add_callback_task(21, 10000, update_left_power);

  right_dir = power_step;
  right_power = 72;
  pinMode(RightWheelPin1, OUTPUT);
  pinMode(RightWheelPin2, OUTPUT);
  digitalWrite(RightWheelPin1, 1);
  digitalWrite(RightWheelPin2, 0);
  right_motor_task = add_motor_task(2, RightWheelPwm, right_power);
  add_callback_task(22, 10000, update_right_power);
}

void loop() {
  run_task();
}
