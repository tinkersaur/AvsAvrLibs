
*** Purpose ***

AvrLog is a tiny library that helps you to debug your code on an
AVR microcontroller.
 

AVR microcontroller has very limited resources, which makes the
programs hard to debug.  If you want to trace your interrupt function,
you cannot even print a string.

The approach of this library is to have a small array which you can
fill with a few different type of messages, and then print them later
when it is convenient for you.

File AvrLog.h is very small and self-explanatory. AvrScheduler (version 2)
is the primary example of using this library (for now).
