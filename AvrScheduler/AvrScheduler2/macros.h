#ifndef MACROS_H
#define MACROS_H

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

#endif
