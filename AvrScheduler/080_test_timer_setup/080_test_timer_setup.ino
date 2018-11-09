/*

    In this file I am testing the manipulation of the timer1 without
    using AvrScheduler library.

    This directory should not contain AvrScheduler.h and AvrScheduler.cpp files.

    Status: I was able to configure timer1 counter overflow interrupt,
    and it is invoked with about correct frequency: every 0.26 seconds,
    which is 2**16*64/16e6.

    TIMER1_COMPA_vect Also works and is being invoked with about right frequency.

*/

#include<Arduino.h>
#include<assert.h>
#include<avr/interrupt.h>
#include<avr/io.h> 

volatile uint16_t timer1_high_count;
volatile int phase=0;
volatile int count=0;

ISR(TIMER1_OVF_vect){
    // This is Timer1 overflow interrupt.

    // Bit TOV1 in TIFR1 is set, when the timer counter
    // overflows, however, Per p 140, TOV1 is automatically
    // cleared when the Timer/Counter1 overflow Interrupt Vector
    // is executed.
    timer1_high_count++;
}

ISR(TIMER1_COMPA_vect){
    count++;
    OCR1A+=3000;
}

void setup(){
  Serial.begin(9600);
  TIMSK1 = 3;  
  TCCR1A = 0;
  TCCR1B = 3;
  OCR1A = 3000;
}

void loop() {
    Serial.print("timer: ");
    Serial.println((unsigned)timer1_high_count);
    Serial.print("counter: ");
    Serial.println((unsigned)count);
    delay(500);
    // if (timer1_high_count<0xFF){
    //     if (phase1==0){
    //         phase1=1;
    //         Serial.print("phase: ");
    //         Serial.println(phase1);
    //         Serial.print("timer: ");
    //         Serial.println(timer1_high_count);
    //         Serial.print("overflow - A: ");
    //         Serial.println(count1);
    //         count1++;
    //         Serial.print("phase: ");
    //         Serial.println(phase1);
    //         delay(1);
    //     }
    // } else
    // if (timer1_high_count<0x1FF){
    //     if (phase1==1){
    //         phase1=0;
    //         Serial.print("overflow B: ");
    //         Serial.println(count1);
    //     }
    // }
}
