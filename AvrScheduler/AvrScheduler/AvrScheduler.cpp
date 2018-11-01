/**  The library for generating PWM and on several pins */

/** Updated on 2018-10-31 */

/*  Clock 16MHz,
 *  For PWM motor:
 *  500Hz, with 255 levels of power yields 78.431 milliseconds. 
 *  For PWM Servo:
 *  1ms-2ms pulse with 255 levels yelds 7.843 milliseconds per level
 *  
 *  Note, everywhere here, "tick" means a 1/64 of milliseconds.
 *  Millisecond would be a convinient choice, but floating point operations
 *  are slow, this precision suffice, and dividing on 64 is easy.
 */
 
#include<Arduino.h>
#include"AvrScheduler.h"
#include<assert.h>

#define error() Serial.print("Scheduler error at "); Serial.println(__LINE__);

// #define TRACE
#ifdef TRACE
    #define ENTER() Serial.print("Entering "); Serial.println(__func__); 
    #define LEAVE() Serial.print("Leaving "); Serial.println(__func__); 
    #define MSG(X) Serial.println(X);
    #define TR(X) Serial.print(#X " = "); Serial.println(X);
    #define REPORT_TASKS() report_tasks();
#else
    #define ENTER()
    #define LEAVE()
    #define MSG(X)
    #define TR(X)
    #define REPORT_TASKS()
#endif
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
  bool clock_overrun;
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

/*** Servo ***/

// These values must be a little bit beyond the range
// to allow for the variation between individaul servos.

static const Ticks MaxServoDuty = 255;
static const Ticks TicksInZeroServoDuty = 550 * 64ul;
    // Per data sheet: 0.001 * 1e6 * 64 = 1000 * 64,
    // Emprirical: 625 * 64.

static const Ticks TicksPerServoDutyLevel = 2000ul * 64 / 255;
    // ^^ Calculated as: 1e6*0.001/255*64 = 251
    // Empirical 2000 * 64 / 255 = 489

static const Ticks ServoPeriodMicroSec = 0.02 * 1e6; // This is 50Hz.

/*** Motor ***/

static const Ticks MaxMotorDuty = 255;

static const Ticks TicksPerMotorDutyLevel = 502ul;
// ^^ Calculated as: 1e6*0.002/255*64

static const Period MotorPeriodMicroSec = 2000ul;
// 1e6*0.002

void schedule_task(TaskIndex);

void report_tasks(){
    Serial.println("  *** Tasks ***");
    Serial.print("next_task: ");
    Serial.println(next_task);
    for(TaskIndex i = 0; i < MaxNumTasks; i++){
        if (tasks[i].mode == NoTask) break;
        Serial.print(i);
        Serial.print(") priority: ");
        Serial.print(tasks[i].priority);
        Serial.print(", wtime: ");
        Serial.print(tasks[i].wtime);
        Serial.print(", prev: ");
        Serial.print(tasks[i].prev);
        Serial.print(", next: ");
        Serial.println(tasks[i].next);
    }
}

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
  if (i >= MaxNumTasks) return i;
  tasks[i].priority = priority;
  tasks[i].mode = CallbackTask;
  tasks[i].wtime = 0;
  CallbackParameters & params = tasks[i].params.callback;
  params.func = func;
  params.period = period;
  schedule_task(next_task);
  return i;
}

TaskIndex add_servo_task(Priority priority, PinIndex pin, Duty initial_duty){
  TaskIndex i = add_task();
  if (i >= MaxNumTasks) return i;
  tasks[i].priority = priority;
  tasks[i].mode = ServoTask;
  tasks[i].wtime = 0;
  PwmParameters & params = tasks[i].params.pwm;
  params.pin = pin;
  pinMode(pin, OUTPUT);
  params.duty = initial_duty;
  params.phase = 0;
  schedule_task(next_task);
  return i;
}

TaskIndex add_motor_task(Priority priority, PinIndex pin, Duty initial_duty){
  TaskIndex i = add_task();
  if (i >= MaxNumTasks) return i;
  tasks[i].priority = priority;
  tasks[i].mode = MotorTask;
  tasks[i].wtime = 0;
  PwmParameters & params = tasks[i].params.pwm;
  params.pin = pin;
  pinMode(pin, OUTPUT);
  params.duty = initial_duty;
  params.phase = 0;
  schedule_task(next_task);
  return i;
}

void calcMotorTaskWtime(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  Period ltime = task.wtime; // last time.
  switch(params.phase){
    case 0:
      params.on_duration = (params.duty * TicksPerMotorDutyLevel) >> 6;
      task.wtime += params.on_duration;
      break;
    case 1:
      task.wtime += MotorPeriodMicroSec - params.on_duration;
      break;
    default:
      error();
      break;
  }
  task.clock_overrun = (task.wtime < ltime); 
}

void calcServoTaskWtime(TaskIndex ti){
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  Period ltime = task.wtime; // last time.
  switch(params.phase){
    case 0:
      // TR(params.duty);
      // TR(TicksInZeroServoDuty);
      // TR(TicksPerServoDutyLevel);
      params.on_duration = (TicksInZeroServoDuty + params.duty * TicksPerServoDutyLevel)>>6;
      // TR(params.en_duration);
      task.wtime += params.on_duration;
      break;
    case 1:
      task.wtime +=  ServoPeriodMicroSec - params.on_duration;
      break;
    default:
      error();
      break;
  }
  task.clock_overrun = (task.wtime < ltime); 
}

void scheduleCallbackTask(TaskIndex ti){
  Task & task = tasks[ti];
  Period ltime = task.wtime; // last time.
  CallbackParameters & params = tasks[next_task].params.callback;
  task.wtime += params.period;
  task.clock_overrun = (task.wtime < ltime); 
}

TaskIndex set_task_duty(TaskIndex id, Duty duty){
  PwmParameters & params = tasks[id].params.pwm;
  params.duty = duty;
}

TaskIndex set_task_wtime(TaskIndex id, Period t){
  tasks[id].wtime = t; 
  schedule_task(id);
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

/* pop the first task and insert it according
 *  to the time and priority
 */

void schedule_task(TaskIndex ti){
  ENTER(); 
  // Pop the first element.
 
  if (ti == next_task){
    next_task = tasks[ti].next;
  }

  TaskIndex bi = tasks[ti].prev;
  TaskIndex ni = tasks[ti].next;
  tasks[bi].next = ni;
  tasks[ni].prev = bi;
  // TR(next_task);

  MSG("Insert the element back according to time and priority.");
  
  while(true){
    if (tasks[ni].mode == NoTask) break;
    if (tasks[ti].clock_overrun > tasks[ni].clock_overrun) break;
    if ( tasks[ni].wtime > tasks[ti].wtime) break;
    if (  tasks[ni].wtime == tasks[ti].wtime
       && tasks[ni].priority < tasks[ti].wtime) break;
    ni = tasks[ni].next;
    if (ni == next_task) break;
    TR(ni);
  }
  MSG("Found a place to insert a task.");
  TR(ni);
  if (ni == next_task){
    next_task = ti; 
  }
  bi = tasks[ni].prev; // the before-task.
  tasks[ti].next=ni;
  tasks[ni].prev=ti;
  tasks[bi].next=ti;
  tasks[ti].prev=bi;
  REPORT_TASKS();
  LEAVE(); 
}

void run_task(){
    ENTER();
    TR(tasks[next_task].wtime);
    while(micros()<tasks[next_task].wtime);
    switch(tasks[next_task].mode){
      case ServoTask:
        calcServoTaskWtime(next_task);
        executePwmTask(next_task);
        break;
      case MotorTask:
        calcMotorTaskWtime(next_task);
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
    schedule_task(next_task);
    LEAVE();
}
