/**  The library for generating PWM and on several pins */

/**
 *  Links:
 *  
 Note that PWM cannot be used for controlling servo.
 PWM has 500Hz and duty cycle 0 to 100%.
 Servo is controlled at 50Hz. and has a duty cycle between 1ms to 2ms.

 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 https://learn.sparkfun.com/tutorials/hobby-servo-tutorial
 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 http://playground.arduino.cc/Main/PWMallPins
 https://www.norwegiancreations.com/2017/09/arduino-tutorial-using-millis-instead-of-delay/
 https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/


 */
/*  Clock 16MHz,
 *  For PWM motor:
 *  500Hz, with 255 levels of power yields 125 clock ticks per level. 
 *  For PWM Servo:
 *  1ms-2ms pulse with 255 levels yelds 62 clock ticks per level
 *  The conclusion is to use divider 64.
 *  Considering how much an interrupt takes, this is at a limit
 *  of what a process can do.
 */

/*
 * TODO:
   - Test if inline keyword works.
   - Figure out which type I can used for Period.
*/

#include"MiniThreads.h"
#include<assert.h>


Task tasks[MaxNumTasks];

#define MaxMotorDuty 255
#define TicksPerMotorDutyLevel ((Period)125)
#define MaxServoDuty 255
#define TicksPerServoDutyLevel ((Period)62)

static const Period TicksInMaxMotorDuty = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Period TicksInMotorPeriod = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Period TicksInServoPeriod = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Period TicksInZeroServoDuty = 0.01 * 16e6;

//assert(TicksInMaxMotorDuty<TicksInMotorPeriod);



TaskIndex next_task;

void init_tasks(){
  next_task = 0;
  for(TaskIndex i = 0; i < MaxNumTasks; i++){
    tasks[i].next = i+1;
    tasks[i].prev = i-1;
    tasks[i].mode = NoTask;
  }
  tasks[0].prev = MaxNumTasks - 1;
  tasks[MaxNumTasks - 1].next = 0;
  tasks[MaxNumTasks - 1].mode = LastTask;
}


TaskIndex add_task(){
}


void scheduleMotorTask(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & param = tasks[next_task].param.pwm;
  switch(param.phase){
    case 0:
      param.on_duration = param.duty * TicksPerMotorDutyLevel;
      task.wtime = task.wtime + param.on_duration;
      break;
    case 1:
      task.wtime = task.wtime + TicksInMotorPeriod - param.on_duration;
      digitalWrite(param.pin, 0);
      break;
  }
}

void scheduleServoTask(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & param = tasks[next_task].param.pwm;
  switch(param.phase){
    case 0:
      param.on_duration = TicksInZeroServoDuty + param.duty * TicksPerServoDutyLevel;
      task.wtime = task.wtime + param.on_duration;
      break;
    case 1:
      task.wtime = task.wtime + TicksInServoPeriod - param.on_duration;
      digitalWrite(param.pin, 0);
      break;
  }
}

/* This works for both Motors and for Servos.
*/
void executePwmTask(TaskIndex ti){
  PwmParameters & param = tasks[next_task].param.pwm;
  switch(param.phase){
    case 0:
      param.phase = 1;
      digitalWrite(param.pin, 1);
      break;
    case 1:
      param.phase = 0;
      digitalWrite(param.pin, 0);
      break;
  }
}

TaskIndex pop_task(){
  TaskIndex ti = next_task; // task index
  TaskIndex bi = tasks[ti].prev;
  next_task = tasks[ti].next;
  tasks[bi].next = next_task;
  tasks[next_task].prev = bi;
  return ti;
}

/* insert the task according to the time and priority
 *  ti -- task index
 */
inline void insert_task(TaskIndex ti){
    TaskIndex pi = next_task; // place index
    while(true){
      if (tasks[pi].mode == NoTask) break; 
      if ( tasks[pi].wtime < tasks[ti].wtime) break;
      if (  tasks[pi].wtime == tasks[ti].wtime
         && tasks[pi].priority < tasks[ti].wtime) break;
      pi = tasks[pi].next;
    }
    TaskIndex bi = tasks[ti].prev; // the before-task.
    tasks[ti].next=pi;
    tasks[pi].prev=ti;
    tasks[bi].next=ti;
    tasks[ti].prev=bi;
}

void run_task(){
    while(millis()<tasks[next_task].wtime);
    // ToDo: Execute the current task, and
    // update the task depending on mode.
    noInterrupts();
    switch(tasks[next_task].mode){
      case ServoTask:
        scheduleServoTask(next_task);
        executePwmTask(next_task);
        break;
      case MotorTask:
        scheduleMotorTask(next_task);
        executePwmTask(next_task);
        break;
    }
    TaskIndex ti=pop_task();
    insert_task(ti);
}
