 *** Purpose ***
 
 An AVR micro-controller has several timers which allows it to
 control two motors or six servos.  The purpose of this library
 is to explore possibility to control many more motors and servos
 by flipping the pins in small functions which are scheduled to
 be invoked in an exactly specified time.

 *** Author ***

(C) 2018 Andrey Sobol. All rights reserved.

 *** Status ***

 Most of functionality is implemented and tested, and the logic
 seems to be correct, but I still see too much jitter.  It could
 be due to some bugs, and I need to test the code carefully.
 Perhaps a little optimization can solve the problem.

 Yet this library can be used to schedule the tasks that are not
 so sensitive to time. (To control servos, I need precision under
 10 microseconds.)
 
 In my first attempt I implemented a task scheduler; it was
 relying on function millis(); I did not go into interrupts and
 timers at that stage.

 I was able to drive 2 motors and a servo, but there was some
 interference between them -- I see that the range of the servo
 swing is not the same as it was without the motors. Should try
 to use different timers.

 Also serial communication interferes with millis() too, and this
 further deteriorated the quality.  The library was intended to
 work together with serial communication.  So this approach has
 failed. An attempt has been made to use Timer1 and interrupts.

 In my second attempt, I relied on timer1 (a 16bit timer). A servo 
 could be controlled in isolation, just fine. Two motors could be
 controlled just fine too. But controlling two motors and a servo
 and an one additional task turned out too much for this little
 chip.
 
 It seems that it did not fail by much, and a small optimization
 might improve the quality significantly.

 However this would still keep MCU very busy, and for the purposes of
 this project it must remain responsive. For my particular
 project it is better just to use an additional MCU.  There are
 other possible future directions which might be more promising;
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

 *** Getting started ***

 For instructions on how to run tests, please read:
 
 AvrScheduler/docs/020_getting_started.txt

 *** Other documentation ***

 This file is intended to be very short. See other documentation in
 folder AvrScheduler/docs/.
  
*** Usage notes ***

- The library has "#define TRACE_SCHEDULER" which
  can switch the code into a mode which is much easier to debug.

*** Notes and Observations ****

- Note that PWM for controlling servo is different from
  controlling a motor.  PWM for motor has 500Hz and duty cycle 0
  to 100%.  Servo is controlled at 50Hz, and has a duty cycle
  between 1ms to 2ms.

- inline keyword is ignored.

- Staggering tasks (using set_task_wtime()) is very important. Almost
  does not work without it, but works well with it. Empirically,
  staggering them by 100-200 is good. The value depends on the
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
  and the motors I have. Need to experiment more with those
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
    - Obvious but most cumbersome option is to have two
      queues: one for motors and one for servos.
    - With the H-bridge that I have, I have observed that (a) it
      might be possible to feed it with 50Hz just fine, and (b)
      it requires much less that 100% duty to exert the full
      power. It might be possible to control it just like the
      servo. Need to read the documentation.
    - Use one board for a bunch of servos and another 
      board for a bunch of motors.

*** Links ***
  

 https://github.com/arduino/Arduino/
 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 https://learn.sparkfun.com/tutorials/hobby-servo-tutorial
 http://playground.arduino.cc/Main/PWMallPins
 https://www.norwegiancreations.com/2017/09/arduino-tutorial-using-millis-instead-of-delay/
 https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
 http://ee-classes.usc.edu/ee459/library/documents/avr_intr_vectors/
 http://www.avr-tutorials.com/interrupts/avr-external-interrupt-c-programming
