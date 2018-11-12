#ifndef AvrScheduler_h
#define AvrScheduler_h

/**  The library for generating PWM and on several pins */

/** Updated on 2018-10-28 12:11 */
#include<stdint.h>


// Uncomment the following line to switch into tracing mode.
// In tracing mode, run_next_task() has to be manually invoked.
// #define TRACE_SCHEDULER

typedef byte TaskIndex;
  // ^^^ Must be big enough to accomodate MaxTaskIndex

typedef byte PinIndex;

typedef unsigned long Period;
  // ^^^ Must be big enough to contain maximum period. The time is measured in milliseconds.

typedef byte Phase;
  // ^^^ A task may have several phases. For example, pin is on, or pin is off.
  
typedef uint16_t Duty;
  // ^^^ A parameter of the task, which is most likely to be
  //     The angle of the servo, or the power of the motor.
  //     By convention, 0 is 0%, and 255 is 100%, for the motor,
  //     and 0 corresponds to the smallest angle, and 255 to the
  //     largest angle. Note that servos might have non-linearities
  //     at the edge of the range, and where this happens might
  //     vary from servo to servo. 
  //     The values must be a little bit beyond the range
  //     to allow for the variation between individaul servos.
  // 
  //     Values 2..250 work well with the servo I have. Please adjust your
  //     angles depending on the servo you have.

typedef byte Priority;

typedef void (*TaskFunc)();

void init_tasks();

/** Use different priority for each task more preductable results. */

TaskIndex add_motor_task(Priority priority, PinIndex pin, Duty initial_duty);

TaskIndex add_servo_task(Priority priority, PinIndex pin, Duty initial_duty);

TaskIndex add_trigger_task(Priority priority, Period period,
    volatile bool * value_ptr);

TaskIndex add_callback_task(Priority priority, Period period, TaskFunc func);

TaskIndex add_delayed_task(Priority priority, Period task_delay, TaskFunc func);

TaskIndex set_task_duty(TaskIndex id, Duty duty);

/**
* Set task's next wakeup time. By default, it is zero.
*/
TaskIndex set_task_wtime(TaskIndex id, Period t);

TaskIndex cancel_task(TaskIndex ti);

/** Tell when the next task is scheduled. */
Period next_task_time();

void wake_tasks();

extern volatile uint16_t timer1_high_count;

// MinServerDuty and MaxServerDuty must be exposed to to account
// for variencies in servos.  A user can go a little bit beyond
// the range to allow for the variation between individaul
// servos.

#define MinServoDuty 160ul
    // Per data sheet: 0.001 * 16e6 / 64 = 250,
    // 155 is an imperical adjustment.

#define MaxServoDuty 625ul
    // To allow pulse variation 0.001 to 0.002 seconds,
    //  MaxServoDuty - MinServoDuty should be calculated as
    // 0.001*16e6/64 = 250, however the current value is chosen
    // imperically to fit he servos that I have.

/** A Quick but inaccurate way of calculating milliseconds.
   This functions does not disable interrupts, therefore, provides
   smother serial io, however, it is prone to errors when overruns occur.
   And yet, these functions are perfect for figuring out time
   while inside of interrupts.
*/
uint32_t quick_millis();
uint32_t quick_micros();


void report_tasks();

#ifdef TRACE_SCHEDULER
    void simulate_compare_interrupt();  
    void run_next_task();
#endif

#endif
