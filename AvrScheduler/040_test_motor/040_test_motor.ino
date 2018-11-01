#include"AvrScheduler.h"

#define    LeftWheelPwm    5
#define    LeftWheelPin2   6
#define    LeftWheelPin1   7

short power, dir;


byte motor_task;

static const short min_power=0;
static const short max_power=255;
short power_step= 1;

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
  power = 128;
  init_tasks();
  
  pinMode(LeftWheelPin1, OUTPUT);
  pinMode(LeftWheelPin2, OUTPUT);
  digitalWrite(LeftWheelPin1, 1);
  digitalWrite(LeftWheelPin2, 0);
  motor_task = add_motor_task(1, LeftWheelPwm, power);
  add_callback_task(2, 10000, update_power);
}

void loop() {
    run_task();
}
