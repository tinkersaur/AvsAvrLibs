*** Purpose ***

AvrLog is a tiny library that helps you to debug your code on an
AVR microcontroller.

AVR microcontroller has very limited resources, which makes the
programs hard to debug.  If you want to trace your interrupt function,
you cannot even print a string.

The approach of this li brary is to have a small array which you can
fill with a few different type of messages, and then print them later
when it is convenient for you.

File AvrLog.h is very small and self-explanatory. AvrScheduler (version 2)
is the primary example of using this library (for now).

Using this library, you can

1) Log a checkpoint to indicate that the line has been executed (and perhaps
   specify the time).

2) Add a very brief message. The message size is limited to 7 characters.

3) Report a value of a variable or an expression. You can specify a 3 character
   symbolic abbreviation for the expression.

4) Check if the log array is full.

5) Print the log array.
