#include"AvrScheduler.h"

#define ServoPin 4

#define LeftWheelPwm    5
#define LeftWheelPin2   6
#define LeftWheelPin1   7

#define RightWheelPin2   8
#define RightWheelPin1   9
#define RightWheelPwm   10

short left_power, right_power, left_dir, right_dir;

byte left_motor_task, right_motor_task;

short angle, dir;

byte servo_task;

static const short min_power=0;
static const short max_power=128;
short power_step= 1;

static const short min_angle=2;
static const short max_angle=250;
short angle_step= 1;

void update_angle(){
    if (angle >= max_angle){
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

   /** left motor setup **/

  left_dir = power_step;
  left_power = 200;
  pinMode(LeftWheelPin1, OUTPUT);
  pinMode(LeftWheelPin2, OUTPUT);
  digitalWrite(LeftWheelPin1, 1);
  digitalWrite(LeftWheelPin2, 0);
  left_motor_task = add_motor_task(1, LeftWheelPwm, left_power);
  add_callback_task(21, 10000, update_left_power);

   /** right motor setup **/

  right_dir = power_step;
  right_power = 72;
  pinMode(RightWheelPin1, OUTPUT);
  pinMode(RightWheelPin2, OUTPUT);
  digitalWrite(RightWheelPin1, 1);
  digitalWrite(RightWheelPin2, 0);
  right_motor_task = add_motor_task(2, RightWheelPwm, right_power);
  add_callback_task(22, 10000, update_right_power);

   /** servo setup **/

  dir = angle_step;
  angle = 128;
  servo_task = add_servo_task(1, ServoPin, angle);
  add_callback_task(2, 10000, update_angle);
}

void loop() {
  run_task();
}
