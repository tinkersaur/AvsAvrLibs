 Purpose
 
 This library enables generating PWM and on several pins to control
 many motors and servos, and run other tasks with a delay or periodically.

 *** Author ***

(C) 2018 Andrey Sobol. All rights reserved.


 *** Status ***

 Some functionality has been implemented and tested, and some is still in
 progress. Most importanly, I am able to drive 2 motors and a servo, although
 there seem to be some interference between them -- I see that the range of the
 servo swing is not the same as it was without the motors. Should try to use
 different timers.

 These tests are working:
 
 010_test_one_thread
 020_test_three_threads
 030_test_servo 
 040_test_motor
 050_test_motors_and_servo
 
 These tests are not working yet:

 060_test_overrun
 070_test_initial_delay
 080_test_cancel_task // Not implemented.

 *** In progress ***

 currently trying to make use of Timer1 and the interrupts to obtain higher
 precision. This work is in AvrScheduler2.
  
 *** TODO ***

- implement add_trigger_task.
- Need to be able to handle the case of empty queue: 
  - I should set up a wake up time somehow
- Expose MinServerDuty and MaxServerDuty
- use the same approach as used in Servo library to generatre better PWM. (in progress).
- Think about a good way to report an error. Right now macro error() reports an error to a serial port.
- Add a function that reports how much time we got before the next task.
- dynamic allocation of tasks.
- in schedule_task(), check if the current position of the task is 
  good, and no reshuffling required.
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
    

*** Observations ****

- inline keyword is ignored.
- The library is intended to work together with serial communication. 
  Relying on millis() however does not generate a good signals.
  Trying to use Timer1. So far so good.
  Also serial communication might be messing with millis() too.
- Staggering tasks (using set_task_wtime())  is important. Almost
  does not work without it, but works well with it.

*** Notes ****

- Arduino documentation says that only two pins can be configured to toggle interrupt.
  However ATMega328P datasheet says differently on page 70. Interrupts PCI0, PCI1, and
  PCI2, can be configured to be triggered if either of the pins in groups of eight
  are toggled. The PCMSK2, PCMSK1 and PCMSK0 Registers control which pins contribute
  to the pin change interrupts. And can be used for waking up from sleep and idle
  modes.

- The timer overflow interrupt needs to be defined as
  ISR(TIMER1_OVF_vect).
  Unfortunately, TIM1_OVF symbol is also defined, and it produces
  unfortunate result of messing up Serial IO. Note that  TIM1_OVF
  is used in MCU datasheet. See link .../avr_intr_vectors below.

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
