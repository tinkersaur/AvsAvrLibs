#ifndef MiniThreads_h
#define MiniThreads_h

/**  The library for generating PWM and on several pins */


#include<stdint.h>

/* Task modes: */
#define ServoTask 2
#define MotorTask 3
#define CallbackTask 4
#define NoTask 0 
#define LastTask 1
  // ^^^ End of the list of the tasks.
  
// The types can be accoomodated for the application needs.

#define MaxNumTasks 10

#define TaskIndex uint8_t 
  // ^^^ Must be big enough to accomodate MaxTaskIndex
  
typedef unsigned long Period;
  // ^^^ Must be big enough to contain maximum period in whatever units.

typedef byte Phase;
  // ^^^ A task may have several phases. For example, pin is on, or pin is off.
  
typedef byte Duty;
  // ^^^ A parameter of the task, which is most likely to be
  //     The angle of the servo, or the power of the motor.
  //     By convention, 0 is 0%, and 255 is 100%.

struct PwmParameters{
    byte phase;
    byte duty;
    byte pin;
    Period on_duration;
}; 

struct CallbackParams{
  void (*func)();
  Period period;
};

union TaskParameters{
  PwmParameters pwm;
  CallbackParams callback;
};

struct Task{
  uint8_t mode;
  short wtime; // wakeup time.
  uint8_t priority;
  TaskParameters param;
  TaskIndex next;
  TaskIndex prev;
};

extern Task tasks[MaxNumTasks];

void init_tasks();

/** Returns a task index, or MaxNumTasks if there is not enough room. **/

TaskIndex add_task();

void run_task();

#endif
