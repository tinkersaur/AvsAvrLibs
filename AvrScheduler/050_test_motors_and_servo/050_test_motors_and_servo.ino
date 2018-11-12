#include"AvrScheduler.h"

#define ServoPin 4

#define LeftWheelPwm    5
#define LeftWheelPin2   6
#define LeftWheelPin1   7

#define RightWheelPin2   8
#define RightWheelPin1   9
#define RightWheelPwm   10

int angle, dir, left_power, right_power, left_dir, right_dir;

TaskIndex left_motor_task, right_motor_task, servo_task;
TaskIndex left_motor_cb_task, right_motor_cb_task, servo_cb_task;

static const short min_power=0;
static const short max_power=40;
short power_step= 1;

static const short min_angle=MinServoDuty;
static const short max_angle=MaxServoDuty;
short angle_step= 10;

void update_angle(){
    angle += dir;
    if (angle >= max_angle){
        angle = max_angle;
        dir = -angle_step;
    }
    if (angle <= min_angle){
        angle = min_angle;
        dir = angle_step;
    }
    set_task_duty(servo_task, angle);
    // Serial.print("angle: ");
    // Serial.println(angle);
}

void update_left_power(){
    left_power += left_dir;
    if (left_power>= max_power){
        left_power = max_power;
        left_dir = -power_step;
    }
    if (left_power <= min_power){
        left_power = min_power;
        left_dir = power_step;
    }
    set_task_duty(left_motor_task, left_power);
}

void update_right_power(){
    right_power += right_dir;
    if (right_power>= max_power){
        right_power = max_power;
        right_dir = -power_step;
    }
    if (right_power <= min_power){
        right_power = min_power;
        right_dir = power_step;
    }
    set_task_duty(right_motor_task, right_power);
}

void update_duty(){
    update_angle();
    update_left_power();
    update_right_power();
}

Period t;
Period n;

void setup(){
  // Initialize this first so that tracing can be done.
  //Serial.begin(9600);
  Serial.begin(115200);
  init_tasks();

   /** servo setup **/

  dir = angle_step;
  angle = (min_angle + max_angle)/2;
  servo_task = add_servo_task(1, ServoPin, angle);
  set_task_wtime(servo_task, 300000);
  servo_cb_task = add_callback_task(2, 20000, update_duty);
  set_task_wtime(servo_cb_task, 300250);

  /** left motor setup **/

  left_dir = power_step;
  left_power = 7;
  pinMode(LeftWheelPin1, OUTPUT);
  pinMode(LeftWheelPin2, OUTPUT);
  digitalWrite(LeftWheelPin1, 1);
  digitalWrite(LeftWheelPin2, 0);
  left_motor_task = add_motor_task(3, LeftWheelPwm, left_power);
  set_task_wtime(left_motor_task, 300500);
  // left_motor_cb_task = add_callback_task(21, 20000, update_left_power);
  // set_task_wtime(left_motor_cb_task, 300150);

  /** right motor setup **/

  right_dir = power_step;
  right_power = 1;
  pinMode(RightWheelPin1, OUTPUT);
  pinMode(RightWheelPin2, OUTPUT);
  digitalWrite(RightWheelPin1, 1);
  digitalWrite(RightWheelPin2, 0);
  right_motor_task = add_motor_task(4, RightWheelPwm, right_power);
  set_task_wtime(left_motor_task, 300750);
  // right_motor_cb_task = add_callback_task(22, 20000, update_right_power);
  // set_task_wtime(right_motor_cb_task, 300250);

  wake_tasks();
}

void loop() {
}
