#ifndef AvrScheduler_h
#define AvrScheduler_h

/**  The library for generating PWM and on several pins */

/** Updated on 2018-10-28 12:11 */
#include<stdint.h>

typedef byte TaskIndex;
  // ^^^ Must be big enough to accomodate MaxTaskIndex

typedef byte PinIndex;

typedef unsigned long Period;
  // ^^^ Must be big enough to contain maximum period. The time is measured in milliseconds.

typedef byte Phase;
  // ^^^ A task may have several phases. For example, pin is on, or pin is off.
  
typedef byte Duty;
  // ^^^ A parameter of the task, which is most likely to be
  //     The angle of the servo, or the power of the motor.
  //     By convention, 0 is 0%, and 255 is 100%.

typedef byte Priority;

typedef void (*TaskFunc)();

void init_tasks();

/** Use different priority for each task more preductable results. */

TaskIndex add_motor_task(Priority priority, PinIndex pin, Duty initial_duty);

TaskIndex add_servo_task(Priority priority, PinIndex pin, Duty initial_duty);

TaskIndex add_callback_task(Priority priority, Period period, TaskFunc func);

TaskIndex add_delayed_task(Priority priority, Period task_delay, TaskFunc func);

TaskIndex set_task_duty(TaskIndex id, Duty duty);

TaskIndex cancel_task(Period period, TaskFunc func);

void run_task();

#endif
