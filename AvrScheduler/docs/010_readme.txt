 Purpose
 
 This library enables generating PWM and on several pins to control
 many motors and servos, and run other tasks with a delay or periodically.

 Author

(C) 2018 Andrey Sobol. All rights reserved.


 Status

 These tests are working:
 
 010_test_one_thread
 020_test_three_threads
 030_test_servo 
 
 These tests are not working yet:

 040_test_motor
 050_test_motors_and_servo
 060_test_overrun
 070_test_initial_delay
 080_test_cancel_task
  
 *** TODO ***

- Why times are off by one?
- Add an initial delay for all add_*_t ask().
- Think about a good way to report an error. Right now macro error() reports an error to a serial port.
- Add a function that reports how much time we got before the next task.
- see how servo library is implemented. Per documentation it is
  able to drive up to 12 servros. See how it is done.
- dynamic allocation of tasks.
- use the same approach as used in Servo library to generatre better PWM.
- other types of tasks:
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

*** Links ***
  
 Note that PWM for controlling servo is different from controlling a motor.
 PWM for motor has 500Hz and duty cycle 0 to 100%.
 Servo is controlled at 50Hz. and has a duty cycle between 1ms to 2ms.

 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 https://learn.sparkfun.com/tutorials/hobby-servo-tutorial
 https://www.arduino.cc/en/Tutorial/SecretsOfArduinoPWM
 http://playground.arduino.cc/Main/PWMallPins
 https://www.norwegiancreations.com/2017/09/arduino-tutorial-using-millis-instead-of-delay/
 https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
