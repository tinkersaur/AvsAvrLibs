 *** Purpose ***
 
 This library enables generating PWM and on several pins to control
 many motors and servos, and run other tasks with a delay or periodically.

 *** Author ***

(C) 2018 Andrey Sobol. All rights reserved.


 *** Status ***

 Most of functionality has been implemented and tested, however
 the desired goal has not been achieved.

 I see a lot of jitter. It could be just due to some bugs.
 I need to read the code carefully. There could be bugs. Also, I
 need to trace it carefully. Look carefully at the point where
 COMPA flag is reset.
 
 I no bugs are to be found, the following would be my conclusion.

 In my first attempt I implemented a task scheduler; it was
 relying on function millis(); I did not go into intterupts and
 timers at that stage.

 I was able to drive 2 motors and a servo, but there was some
 interference between them -- I see that the range of the servo
 swing is not the same as it was without the motors. Should try
 to use different timers.

 Also serial communication might be messing with millis() too,
 and this further deteriorated the quality.  An attempt has been
 made to use Timer1 and interrupts.

 The library was intended to work together with serial communication. 
 So this approach has failed.

 In my second attempt, I relied on timer1 (a 16bit timer). A servo 
 could be controlled in isolation, just fine. Two motors could be
 controlled just fine too. But controlling two motors and a servo
 and an one additional task turned out too much for this little
 chip.
 
 It seems that it did not fail by much, and a small otpimization
 might improve the quality significantly.

 However this would still keep MCU very busy, and for the purposes of
 this project it must remain responsive. For my particular
 project it is better just to use an additonal MCU.  There are
 other possible future directions which might be more promissing;
 see below.

 *** Tests ***

 These tests have been working with the first and the second version:
 
 010_test_one_thread
 020_test_three_threads
 025_test_trigger
 030_test_servo   // This is just a proof-of-principle
                  // prototype for test_servo_task.
 035_test_servo_task
 040_test_motor
 050_test_motors_and_servo
 
 These tests have not been implemented yet:

 060_test_overrun
 070_test_initial_delay
 080_test_cancel_task // Not implemented.
 090_test_empty_queue // Not implemented.

  
 *** TODO ***

- read the code carefully. there could be bugs. trace it
  carefully.
- print tasks in the order they will be invoked.
- improve logging by allowing a value to be reported.
- Motor with duty level 0% or 100% could be excluded from the queue.
- Think about a good way to report an error. Right now macro error() reports an error to a serial port.
- Add a function that reports how much time we got before the next task.
- dynamic allocation of tasks.
- in schedule_task(), check if the current position of the task is 
  good, and no reshuffling required.
- Driving motors with 100Hz PWM works just fine with the H-Bridge and
  the motors I have. However playing with these numbers lead to unexpected
  results. for example: period=5000 and step=20 caused a lot of problems.
  Need to play more with these constants. 
- other types of tasks that can be implemented:
    trigger_boolean:
        : period
        : address of a boolean.
    swipe servo
        : pin
        : min angle
        : max angle
        : swing duration
        : pause
        : offset.
    servo motions:
        : pin
        : num_motion_modes ... mode
        : num_motion_phases[mode]... phase
        : position[mode][phase]
        : time[mode][phase]
        : offset
    read_analog:
        : pin
        : id
        : get_task_value(task_id).
    
*** Usage notes ***

- The library has "#define TRACE_SCHEDULER" which
can switch the code into a mode which is much easier to debug.

*** Notes, and Observations ****

- inline keyword is ignored.

- Staggering tasks (using set_task_wtime()) is very important. Almost
  does not work without it, but works well with it. Emperically,
  stuggering them by 100-200 is good. The value depends on the
  size of the queue.

- Arduino documentation says that only two pins can be configured
  to toggle interrupt.  However ATMega328P datasheet says
  differently on page 70. Interrupts PCI0, PCI1, and PCI2, can be
  configured to be triggered if either of the pins in groups of
  eight are toggled. The PCMSK2, PCMSK1 and PCMSK0 Registers
  control which pins contribute to the pin change interrupts. And
  can be used for waking up from sleep and idle modes.

- The timer overflow interrupt needs to be defined as
  ISR(TIMER1_OVF_vect).
  Unfortunately, TIM1_OVF symbol is also defined, and it produces
  unfortunate result of messing up Serial IO. Note that  TIM1_OVF
  is used in MCU datasheet. See link .../avr_intr_vectors below.
  Be careful to not to confuse these two symbols.

- Driving motors with 100Hz PWM works just fine with the H-Bridge
  and the motors I have. Need to experiemnt more with those
  numbers.

*** Future directions ***

- Keep executing tasks, if their time to execute has come, and
  only after that, calculate their next wakeup time and sort them
  back into the list of the scheduled tasks.

- Resorting the task is the deal breaker. It would be great
  if we can just have them in a loop. Resorting would only
  occur when the duty changes. The problem however is that
  the motors and servos have different main frequencies.
  This can be remedied though in several ways:
    - Obviouls but most cumbersome option is to have two
      queues: one for motors and one for servos.
    - With the H-bridge that I have, I have observed that
      (a) it might be possible to feed it with 50Hz just
      fine, and (b) it requires much less that 100% duty
      to exert the full power. It might be possible to
      control it just like the servo. Need to read the
      documentation.
    - Use one board for a bunch of servos and another 
      board for a bunch of motors.

*** Links ***
  
 Note that PWM for controlling servo is different from controlling a motor.
 PWM for motor has 500Hz and duty cycle 0 to 100%.
 Servo is controlled at 50Hz. and has a duty cycle between 1ms to 2ms.

 https://github.com/arduino/Arduino/
 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 https://learn.sparkfun.com/tutorials/hobby-servo-tutorial
 http://playground.arduino.cc/Main/PWMallPins
 https://www.norwegiancreations.com/2017/09/arduino-tutorial-using-millis-instead-of-delay/
 https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
 http://ee-classes.usc.edu/ee459/library/documents/avr_intr_vectors/
 http://www.avr-tutorials.com/interrupts/avr-external-interrupt-c-programming
