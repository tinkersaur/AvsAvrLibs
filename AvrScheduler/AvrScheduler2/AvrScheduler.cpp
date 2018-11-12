
/**  The library for generating PWM and on several pins */

/** Updated on 2018-11-08  */

#include<Arduino.h>
#include"AvrScheduler.h"
#include<assert.h>
#include<avr/interrupt.h>
#include<avr/io.h> 
#include"AvrLog.h"

#define error() Serial.print("Scheduler error at "); Serial.println(__LINE__);

#ifdef TRACE_SCHEDULER
    #define TRACE
#else
    #define SETUP_ISR
#endif

#include"macros.h"

#define ADD_LOG(timestamp, value) checkpoint(timestamp, value) 
#define CHECKPOINT() checkpoint(quick_micros(), __LINE__) 
#define TR2(key, value) trace(key, value) 


typedef unsigned long Ticks; 
    // ^^^ A tick is a counter of Timer1,
    // which is downscaled from the clock with ratio 64.

struct PwmParameters{
    Phase phase;
    Duty duty;
    byte pin;
    Ticks on_duration;
}; 

struct TriggerParameters{
  volatile bool * value;
  Period period;
};

struct CallbackParameters{
  void (*func)();
  Period period;
};

union TaskParameters{
  PwmParameters pwm;
  TriggerParameters trigger;
  CallbackParameters callback;
};

/* Task modes: */
enum TaskType{
  NoTask, // Empty slot which can be used.
  ServoTask, 
  MotorTask,
  TriggerTask,
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

volatile uint16_t timer1_high_count;

// Note that the last task is the task t such that t.next == next_task.

/*  Clock 16MHz,
 *  For PWM motor:
 *  500Hz, with 255 levels of power yields 78.431 milliseconds. 
 *  Prescaler 64 yields 19 ticks.
 *  For PWM Servo:
 *  1ms-2ms pulse with 255 levels yelds 7.843 milliseconds per level
 *  Prescaler 64 yields 1 tick.
 *  
 *  Note, everywhere here, "tick" means a 4 milliseconds, which is 64
 *  clock ticks.
 *  Millisecond would be a convinient choice, but floating point operations
 *  are slow, this precision suffice, and dividing on 64 is easy.
 */
 
/*** Servo ***/

static const Ticks TicksPerServoDutyLevel = 1ul;
    // ^^ Calculated as: 0.001/250*16e6/64 = 1

static const Ticks ServoPeriod = 5000ul; // This is 50Hz.
    // ^^ calculated as 0.02*16e6/64 = 5000

/*** Motor ***/

static const Ticks MaxMotorDuty = 250;

static const Ticks MotorPeriod = 2500ul;
    //^^^ Calculated as 16e6/64*0.01

static const Ticks TicksPerMotorDutyLevel = 10ul;
// ^^ Calculated as: MotorPeriod/MaxMotorDuty = 10.

static void schedule_task(TaskIndex);
void run_next_task();
static void calcMotorTaskWtime(TaskIndex ti);
static void calcServoTaskWtime(TaskIndex ti);
static void calcTriggerTaskWtime(TaskIndex ti);
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

Period next_task_time(){
    return tasks[next_task].wtime<<2; 
}

void wake_tasks(){
   // assert(OCF1A == 1);
   // ENTER();
   // TR(TIFR1);
   TIFR1 |= (1<<OCF1A); 
}

uint32_t quick_millis(){
    return ((((uint32_t)timer1_high_count)<<16) | TCNT1)/250; 
}

uint32_t quick_micros(){
    return ((((uint32_t)timer1_high_count)<<16) | TCNT1)*4; 
}

static void setup_timers(){
    // clear overflow just for good measure:
    cli();
    TIFR1 &= ~(1<<OCF1A);
    TIFR1 &= ~(1<<OCF1A);
    TIFR1 &= ~(1<<TOV1);
    TCNT1 = 0;

    #ifdef SETUP_ISR
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

    #else
        TIMSK1 = 0;  
        TCCR1A = 0;
        TCCR1B = 0;
    #endif

    timer1_high_count = 0;
    sei();
}


#ifdef SETUP_ISR
ISR(TIMER1_OVF_vect){
    // This is Timer1 overflow interrupt.

    // Bit TOV1 in TIFR1 is set, when the timer counter
    // overflows, however, Per p 140, TOV1 is automatically
    // cleared when the Timer/Counter1 overflow Interrupt Vector
    // is executed.
    timer1_high_count++;
}
#endif

#ifdef SETUP_ISR

    /* ISR() defines an interrupt vector*/
    /* Timer1 CompareA interrupt.*/
    ISR(TIMER1_COMPA_vect)
#else
    void simulate_compare_interrupt()  
#endif
{
        
    // Bit OCF1A in TIFR1 is set, when the timer counter
    // equals to OCR1A, however, Per p 140, OCF1A is automatically
    // cleared when the Timer/Counter1 overflow Interrupt Vector
    // is executed.

    // Note that TIMER1_COMPA_vect has higher priority than
    // TIMER1_OVF_vect therefore we do not need to worry about
    // disabling interrupts. However we need to check if
    // the overflow occured, and increment the time1_high_count

    // ENTER();
    CHECKPOINT();

    bool overflow;
    uint32_t clock;

    //assert(TOV1 ==0);

    // TR(TIFR1);
    // ADD_LOG(quick_millis(), 77010);

    if (TIFR1 & (1<<TOV1)){
        // MSG("Overflow has occured:");
        // ADD_LOG(quick_millis(), 77015);
        TIFR1 &=~(((uint8_t)1)<<TOV1);
        timer1_high_count++;
    }
    // Check if overflow occured.
    if (timer1_high_count >= 0x8FFF){
        overflow = true;
        timer1_high_count -= 0x8FFF;
    }

    // Obtain the true clock value. 
    clock = (((uint32_t)timer1_high_count)<<16) | TCNT1;

    // Even if it is not time to run the next task, we
    // must setup the next wakeup time correctly.
    bool done = false;
    bool time_set = false;
    while(!done){
        done = true;
        // If overflow occureed, update times of all tasks:
        if (overflow){
            overflow = false;
            TaskIndex ti=next_task;
            do{
                if (tasks[ti].wtime >= 0x8FFFul<<16){
                    tasks[ti].wtime -= 0x8FFFul<<16; 
                }
                ti = tasks[ti].next;
            }while(tasks[ti].mode!=NoTask && ti!=next_task);
        }

        // While it is time to run the top task:

        // ADD_LOG(quick_millis(), 77020);
        // TR2("WT", tasks[next_task].wtime);
        if (  tasks[next_task].mode != NoTask
           && tasks[next_task].wtime <= clock){

            // CHECKPOINT();
            // ADD_LOG(quick_millis(), clock);
            // MSG("It is time to run the next task:");
            // TR(clock);
            run_next_task();
            done = false;
            time_set = false;
        } else {
            // CHECKPOINT();
        }

        // In the rest of the loop body, we evaluate
        // if it is time to run the next task again.

        if (tasks[next_task].mode != NoTask && !time_set){
            // Clear interrupt flag. Page 114:
            TIFR1 &= ~(1<<OCF1A);

            // MSG("Setting up the next wakeup time:");
            // TR(tasks[next_task].wtime);

            // Set wakeup time:
            OCR1A = tasks[next_task].wtime & 0xFFFF; 

            // Check if overflow occured.
            if (timer1_high_count >= 0x8FFF){
                overflow = true;
                timer1_high_count -= 0x8FFF;
            }

            // Obtain the true clock value. 
            clock = (timer1_high_count<<16) | TCNT1;
            time_set = true;
        }
    }
    // ADD_LOG(quick_millis(), 77050);
    // LEAVE();
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
  current_log = 0;
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

TaskIndex add_trigger_task(Priority priority, Period period, volatile bool * value_ptr){
  TaskIndex i = add_task();
  if (i >= MaxNumTasks) return i;
  tasks[i].priority = priority;
  tasks[i].mode = TriggerTask;
  tasks[i].wtime = 0;
  TriggerParameters & params = tasks[i].params.trigger;
  params.value = value_ptr;
  params.period = period;
  schedule_task(i);
  return i;
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
  // ENTER();
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  // TR(task.wtime);
  // TR(params.phase);
  switch(params.phase){
    case 1:
      params.on_duration = params.duty * TicksPerMotorDutyLevel;
      task.wtime += params.on_duration;
      break;
    case 0:
      task.wtime += MotorPeriod - params.on_duration;
      break;
    default:
      error();
      break;
  }
  // TR(task.wtime);
  // LEAVE();
}

void calcServoTaskWtime(TaskIndex ti){
  // ENTER();
  Task & task = tasks[ti];
  PwmParameters & params = tasks[next_task].params.pwm;
  // TR(params.phase);
  switch(params.phase){
    case 1:
      TR2("DU", params.duty);
      // TR(MinServoDuty);
      // TR(TicksPerServoDutyLevel);
      params.on_duration = params.duty;
      // TR(params.en_duration);
      task.wtime += params.on_duration;
      break;
    case 0:
      TR2("DU", params.duty);
      task.wtime +=  ServoPeriod - params.on_duration;
      break;
    default:
      error();
      break;
  }
  // LEAVE();
}

void calcTriggerTaskWtime(TaskIndex ti){
  // ENTER();
  Task & task = tasks[ti];
  TriggerParameters & params = tasks[next_task].params.trigger;
  // TR(task.wtime);
  task.wtime += (params.period>>2);
  // TR(task.wtime);
  // LEAVE();
}

void calcCallbackTaskWtime(TaskIndex ti){
  // ENTER();
  Task & task = tasks[ti];
  CallbackParameters & params = tasks[next_task].params.callback;
  // TR(task.wtime);
  task.wtime += (params.period>>2);
  // TR(task.wtime);
  // LEAVE();
}

TaskIndex set_task_duty(TaskIndex id, Duty duty){
  PwmParameters & params = tasks[id].params.pwm;
  params.duty = duty;
}

TaskIndex set_task_wtime(TaskIndex id, Period t){
  tasks[id].wtime = t>>2; 
  schedule_task(id);
}

/** This works for both Motors and for Servos.
*/
void executePwmTask(TaskIndex ti){
  PwmParameters & params = tasks[next_task].params.pwm;
  switch(params.phase){
    case 1:
      params.phase = 0;
      digitalWrite(params.pin, 0);
      TR2("PN", 0);
      break;
    case 0:
      params.phase = 1;
      digitalWrite(params.pin, 1);
      TR2("PN", 1);
      break;
  }
}

/** Pop the task and insert it according to its time and priority.
 */
void schedule_task(TaskIndex ti){
  // ENTER(); 
  // Pop the first element.
 
  if (ti == next_task){
    next_task = tasks[ti].next;
  }

  TaskIndex bi = tasks[ti].prev;
  TaskIndex ni = tasks[ti].next;
  tasks[bi].next = ni;
  tasks[ni].prev = bi;
  // TR(next_task);

  // MSG("Insert the element back according to time and priority.");
 
  bool last_task = false;
  ni = next_task;
  while(true){
    if (tasks[ni].mode == NoTask) break;
    // Sometimes we need to insert a NoTask. But in this
    // case we do not care about the time and the priority.
    if (tasks[ti].mode != NoTask) {
        if ( tasks[ni].wtime > tasks[ti].wtime) break;
        if (  tasks[ni].wtime == tasks[ti].wtime
           && tasks[ni].priority > tasks[ti].priority) break;
    }
    ni = tasks[ni].next;
    if (ni == next_task) {
        last_task = true;
        break;
    }
    // TR(ni);
  }
  // MSG("Found a place to insert a task.");
  // TR(ni);
  if (ni == next_task && !last_task){
    next_task = ti; 
  }
  bi = tasks[ni].prev; // the before-task.
  tasks[ti].next=ni;
  tasks[ni].prev=ti;
  tasks[bi].next=ti;
  tasks[ti].prev=bi;
  // REPORT_TASKS();
  // LEAVE(); 
}

TaskIndex cancel_task(TaskIndex ti){
    tasks[ti].mode = NoTask;
    schedule_task(ti);
}
void wait_for_task(){
    // ENTER();
    // TR(tasks[next_task].wtime);
    while(micros()<tasks[next_task].wtime);
    
}

void run_next_task(){
    switch(tasks[next_task].mode){
      case ServoTask:
        executePwmTask(next_task);
        calcServoTaskWtime(next_task);
        break;
      case MotorTask:
        executePwmTask(next_task);
        calcMotorTaskWtime(next_task);
        break;
      case TriggerTask:
        //ADD_LOG(quick_micros(), 3);
        *(tasks[next_task].params.trigger.value)=true;
        calcTriggerTaskWtime(next_task);
        break;
      case CallbackTask:
        //ADD_LOG(quick_micros(), 3);
        tasks[next_task].params.callback.func();
        calcCallbackTaskWtime(next_task);
        break;
      case NoTask:
        return;
      default:
        // error.
        return;
    }
    schedule_task(next_task);
    // LEAVE();
}

