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
 *  500Hz, with 255 levels of power yields 78.431 milliseconds. 
 *  For PWM Servo:
 *  1ms-2ms pulse with 255 levels yelds 7.843 milliseconds per level
 *  
 *  Note, everywhere here, "tick" means a 1/64 of milliseconds.
 *  Millisecond would be a convinient choice, but floating point operations
 *  are slow, this precision suffice, and dividing on 64 is easy.
 *  
 *  
 */
 
/*
 * TODO:
   - handle the situation when clock overflows.
   - Test if inline keyword works.
   - Figure out which type I can used for Period.
   - Think about a good way to report an error. Right now macro error() reports an error to a serial port.
   - Most functions could be inlined.
*/

#include<Arduino.h>
#include"AvrScheduler.h"
#include<assert.h>

#define error() Serial.print("Scheduler error at "); Serial.println(__LINE__);
#define ENTER() Serial.print("Entering "); Serial.println(__func__); 
#define MSG(X) Serial.println(X);
#define TR(X) Serial.print(#X " = "); Serial.println(X);

struct PwmParameters{
    Phase phase;
    Duty duty;
    byte pin;
    Period on_duration;
}; 

struct CallbackParameters{
  void (*func)();
  Period period;
};

union TaskParameters{
  PwmParameters pwm;
  CallbackParameters callback;
};

/* Task modes: */
enum TaskType{
  NoTask, // Empty slot which can be used.
  ServoTask, 
  MotorTask,
  CallbackTask
};
  
// The types can be accoomodated for the application needs.

#define MaxNumTasks 10

struct Task{
  uint8_t mode;
  Period wtime; // wakeup time.
  uint8_t priority;
  TaskParameters params;
  TaskIndex next;
  TaskIndex prev;
};

extern Task tasks[MaxNumTasks];

Task tasks[MaxNumTasks];

TaskIndex next_task;

// Note that the last task is the task t such that t.next == next_task.

typedef unsigned long Ticks; // 1/64 of millisecond

static const Ticks MaxMotorDuty = 255;

static const Ticks TicksPerMotorDutyLevel = 502;
// ^^ Calculated as: 1e6*0.002/255*64

static const Ticks MaxServoDuty = 255;
static const Ticks TicksPerServoDutyLevel = 251;
// ^^ Calculated as: 1e6*0.001/255*64

static const Ticks TicksInMaxMotorDuty = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Ticks TicksInMotorPeriod = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Ticks TicksInServoPeriod = MaxMotorDuty * TicksPerMotorDutyLevel;
static const Ticks TicksInZeroServoDuty = 0.01 * 16e6;

//assert(TicksInMaxMotorDuty<TicksInMotorPeriod);


TaskIndex pop_task();
void insert_task(TaskIndex ti);

void init_tasks(){
  // ENTER();
  next_task = 0;
  for(TaskIndex i = 0; i < MaxNumTasks; i++){
    tasks[i].next = (i+1) % MaxNumTasks;
    tasks[i].prev = i-1;
    tasks[i].mode = NoTask;
    // TR(i);
    // TR(tasks[i].next);
    // TR(tasks[i].prev = i-1);
  }
  tasks[0].prev = MaxNumTasks - 1;
}

/** Returns a task index, or MaxNumTasks if there is not enough room. **/

TaskIndex add_task(){
  TaskIndex i = next_task;
  while(true){
    if (tasks[i].mode == NoTask){
      return i;  
    }
    i = tasks[i].next;
    if (i == next_task) return MaxNumTasks;
  }
}

TaskIndex add_callback_task(Priority priority, Period period, TaskFunc func){
  TaskIndex i = add_task();
  tasks[i].priority = priority;
  tasks[i].mode = CallbackTask;
  tasks[i].wtime = 0;
  CallbackParameters & params = tasks[i].params.callback;
  params.func = func;
  params.period = period;
  // i=pop_task();
  //insert_task(i);
}



void scheduleMotorTask(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  switch(params.phase){
    case 0:
      params.on_duration = (params.duty * TicksPerMotorDutyLevel) >> 6;
      task.wtime = task.wtime + params.on_duration;
      break;
    case 1:
      task.wtime = task.wtime + TicksInMotorPeriod - params.on_duration;
      break;
    default:
      error();
      break;
  }
}

void scheduleServoTask(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  switch(params.phase){
    case 0:
      params.on_duration = (TicksInZeroServoDuty + params.duty * TicksPerServoDutyLevel)>>6;
      task.wtime = task.wtime + params.on_duration;
      break;
    case 1:
      task.wtime = task.wtime + TicksInServoPeriod - params.on_duration;
      break;
    default:
      error();
      break;
  }
}

void scheduleCallbackTask(TaskIndex ti){
  Task & task = tasks[ti];
  CallbackParameters & params = tasks[next_task].params.callback;
  task.wtime = task.wtime + params.period;
}

/* This works for both Motors and for Servos.
*/
void executePwmTask(TaskIndex ti){
  PwmParameters & params = tasks[next_task].params.pwm;
  switch(params.phase){
    case 0:
      params.phase = 1;
      digitalWrite(params.pin, 1);
      break;
    case 1:
      params.phase = 0;
      digitalWrite(params.pin, 0);
      break;
  }
}

TaskIndex pop_task(){
  TaskIndex ti = next_task; // task index
  TaskIndex bi = tasks[ti].prev;
  next_task = tasks[ti].next;
  tasks[bi].next = next_task;
  tasks[next_task].prev = bi;
  // TR(next_task);
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
      // TR(pi);
    }
    // MSG("Foun a place to insert a task.");
    // TR(pi);
    TaskIndex bi = tasks[pi].prev; // the before-task.
    if (pi == next_task){
      next_task = ti; 
    }
    tasks[ti].next=pi;
    tasks[pi].prev=ti;
    tasks[bi].next=ti;
    tasks[ti].prev=bi;
}

void run_task(){
    //TR(tasks[next_task].wtime);
    while(micros()<tasks[next_task].wtime);
    switch(tasks[next_task].mode){
      case ServoTask:
        scheduleServoTask(next_task);
        executePwmTask(next_task);
        break;
      case MotorTask:
        scheduleMotorTask(next_task);
        executePwmTask(next_task);
        break;
      case CallbackTask:
        scheduleCallbackTask(next_task);
        tasks[next_task].params.callback.func();
        break;
      case NoTask:
        return;
      default:
        // error.
        return;
    }
    TaskIndex ti=pop_task();
    insert_task(ti);
}
