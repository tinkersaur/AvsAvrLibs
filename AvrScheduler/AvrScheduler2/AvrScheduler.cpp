
/**  The library for generating PWM and on several pins */

/** Updated on 2018-11-08  */

#include<Arduino.h>
#include"AvrScheduler.h"
#include<assert.h>
#include<avr/interrupt.h>
#include<avr/io.h> 

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

volatile uint16_t timer1_high_count;

// Note that the last task is the task t such that t.next == next_task.

typedef unsigned long Ticks; // A tick is a counter of Timer1,
                             // which is downscaled from the clock with ratio 64.

/*  Clock 16MHz,
 *  For PWM motor:
 *  500Hz, with 255 levels of power yields 78.431 milliseconds. 
 *  Prescaler 64 yields 19 ticks.
 *  For PWM Servo:
 *  1ms-2ms pulse with 255 levels yelds 7.843 milliseconds per level
 *  Prescaler 64 yields 1 tick.
 *  
 *  Note, everywhere here, "tick" means a 1/64 of milliseconds.
 *  Millisecond would be a convinient choice, but floating point operations
 *  are slow, this precision suffice, and dividing on 64 is easy.
 */
 
/*** Servo ***/

// These values must be a little bit beyond the range
// to allow for the variation between individaul servos.

static const Ticks MaxServoDuty = 255;
static const Ticks TicksInZeroServoDuty = 255ul;
    // Per data sheet: 0.001 * 16e6 / 64 = 250,
    // However we tweak so that to 
    // TicksInZeroServoDuty + MaxServoDuty * TicksPerServoDutyLevel  == MotorPeriod == 255*2.

static const Ticks TicksPerServoDutyLevel = 1ul;
    // ^^ Calculated as: 0.001/255*16e6/64 = 0.98

static const Ticks ServoPeriod = 510ul; // This is 50Hz.
    // ^^ calculated as 0.002*16e6/64 = 500, but taking into account
    // round off errors, we set it to 505.

/*** Motor ***/

static const Ticks MaxMotorDuty = 255;

static const Ticks TicksPerMotorDutyLevel = 2ul;
// ^^ Calculated as: 1e6*0.002/255*64

static const Period MotorPeriod = 510ul;
// 1e6*0.002

static void schedule_task(TaskIndex);
static void run_next_task();
static void calcMotorTaskWtime(TaskIndex ti);
static void calcServoTaskWtime(TaskIndex ti);
static void calcCallbackTaskWtime(TaskIndex ti);

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

/*
Questions:
    - If I assign this to timer compare interrupt, this interrupt
      will be disabled until the function returns?

    Links:
    http://www.avr-tutorials.com/interrupts/avr-external-interrupt-c-programming
*/

// TSR(TIMER1COMPA) ???

static void setup_timers(){
    // Configure Timer1:

    // TIMSK1 = [ – | – | ICIE1 | – | – | OCIE1B | OCIE1A | TOIE1 ] -- p 139.
    // ICIE1 = 0 -- Timer/Counter1, Input Capture Interrupt Enable -- p 139
    // OCIE1A = 1 -- Timer/Counter1, Output Compare A Match Interrupt Enable -- p139
    // OCIE1B = 0 -- Timer/Counter1, Output Compare B Match Interrupt Enable -- p139
    // TOIE1 = 1 Timer/Counter1, Overflow Interrupt Enable -- p139

    TIMSK1 = 3;  
    
    // Next we configure clock source for timer 1, and
    // prescaler for timer1 to be 1/64.

    // COM1A1 = COM1B1 = COM1A0 = COM1B0 = 0 -- OC1A/B disconnected. P 134.
    // WGM1[3:0] = 0 -- Normal Mode. p125, p136
    // ICNC1=0 - Select noise caneling. Not important. P 121
    // ICES1=0 - select trigger edge. Not important. P 136.
    // CS1[2:0] = 0b011  -- set clock prescaler to 1/64, p 137
    // TCCR1A = [COM1A1 | COM1A0 | COM1B1 | COM1B0 | –     | –    | WGM11 | WGM10] -- p134
    // TCCR1B = [ ICNC1 | ICES1  | –      |  WGM13 | WGM12 | CS12 | CS11  | CS10 ] -- p136

    TCCR1A = 0;
    TCCR1B = 3;

}

ISR(TIMER1_OVF_vect){
    // This is Timer1 overflow interrupt.

    // Bit TOV1 in TIFR1 is set, when the timer counter
    // overflows, however, Per p 140, TOV1 is automatically
    // cleared when the Timer/Counter1 overflow Interrupt Vector
    // is executed.
    timer1_high_count++;
}


/* ISR() defines an interrupt vector*/
/* Timer1 CompareA interrupt.*/
ISR(TIMER1_COMPA_vect){
        
    // Bit OCF1A in TIFR1 is set, when the timer counter
    // equals to OCR1A, however, Per p 140, OCF1A is automatically
    // cleared when the Timer/Counter1 overflow Interrupt Vector
    // is executed.

    unsigned char sreg;
    bool etime; // Execute time. 
    // etime is true if it is time to execute the new task.
    bool overflow;
    unsigned long clock;

    // We want to obtain the true clock value. To do that, we temporaraly
    // disable all interrupts, so that overflow interrupt does not occur.
    sreg = SREG;
    cli();

    // Check if overflow occured.
    if (timer1_high_count >= 0x8FFF){
        overflow = true;
        timer1_high_count -= 0x8FFF;
    }

    // Obtain the true clock value. 
    clock = (timer1_high_count<<16) | TCNT1;

    // Restore interrupts:
    SREG = sreg; 

    // While it is time to run the top task:
    while(tasks[next_task].wtime >= clock){

        // If overflow occureed, update times of all tasks:
        if (overflow){
            TaskIndex ti=next_task;
            do{
                tasks[ti].wtime -= 0x8FFF<<16; 
                ti = tasks[ti].next;
            }while(ti!=next_task);
        }

        run_next_task();

        // In the rest of the loop body, we evaluate
        // if it is time to run the next task again.

        SREG = sreg;
        cli();

        // Clear interrupt flag. Page 114:
        TIFR1 &= ~(1<<TIMSK1);

        // Set wakeup time:
        OCR1A = tasks[next_task].wtime & 0xFFFF; 

        // Check if overflow occured.
        if (timer1_high_count >= 0x8FFF){
            overflow = true;
            timer1_high_count -= 0x8FFF;
        }

        // Obtain the true clock value. 
        clock = (timer1_high_count<<16) | TCNT1;

        // restore interrupts.
        SREG = sreg;
    }
}


void init_tasks(){
  // ENTER();
  setup_timers();
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
      params.on_duration = params.duty * TicksPerMotorDutyLevel;
      task.wtime += params.on_duration;
      break;
    case 1:
      task.wtime += MotorPeriod - params.on_duration;
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
      params.on_duration = TicksInZeroServoDuty + params.duty * TicksPerServoDutyLevel;
      // TR(params.en_duration);
      task.wtime += params.on_duration;
      break;
    case 1:
      task.wtime +=  ServoPeriod - params.on_duration;
      break;
    default:
      error();
      break;
  }
  task.clock_overrun = (task.wtime < ltime); 
}

void calcCallbackTaskWtime(TaskIndex ti){
  Task & task = tasks[ti];
  Period ltime = task.wtime; // last time.
  CallbackParameters & params = tasks[next_task].params.callback;
  task.wtime += (params.period>>2);
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
    if (tasks[ti].mode != NoTask) {
        if (tasks[ti].clock_overrun > tasks[ni].clock_overrun) break;
        if ( tasks[ni].wtime > tasks[ti].wtime) break;
        if (  tasks[ni].wtime == tasks[ti].wtime
           && tasks[ni].priority < tasks[ti].wtime) break;
    }
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

TaskIndex cancel_task(TaskIndex ti){
    tasks[ti].mode = NoTask;
    schedule_task(ti);
}
void wait_for_task(){
    ENTER();
    TR(tasks[next_task].wtime);
    while(micros()<tasks[next_task].wtime);
    
}

void run_next_task(){
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
        calcCallbackTaskWtime(next_task);
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

