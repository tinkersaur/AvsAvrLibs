#include<string.h>
#include<Arduino.h>
#include"AvrLog.h"


LogRecord logs[MaxNumLogs];
uint8_t current_log;

void checkpoint(uint32_t timestamp, int16_t line_number){ 
    if (current_log >= MaxNumLogs) return;
    CheckpointRec & rec = logs[current_log].rec.checkpoint;
    logs[current_log].mode = CheckpointLogMode;
    rec.timestamp = timestamp;
    rec.line=line_number;
    current_log++;
}

void trace(char * key, int val){
    if (current_log >= MaxNumLogs) return;
    IntValueRec & rec = logs[current_log].rec.int_value;
    logs[current_log].mode = IntValueLogMode;
    strncpy(rec.key, key, 3);
    rec.value = val;
    current_log++;
}

bool logs_full(){
    return current_log >= MaxNumLogs;
}

void report_logs(){
    Serial.println("*** Logs ***");
    for(uint8_t i=0; i<current_log; i++){
        switch(logs[i].mode){
            case CheckpointLogMode:
                {
                    CheckpointRec & rec = logs[i].rec.checkpoint;
                    Serial.print(rec.timestamp);
                    Serial.print(" : @ line ");
                    Serial.println(rec.line);
                    break;
                }
            case IntValueLogMode:
                {
                    IntValueRec & rec = logs[i].rec.int_value;
                    for(uint8_t i = 0; i<3 && rec.key[i]!=0 ; i++){
                        Serial.write(rec.key[i]);
                    }
                    Serial.print(" = ");
                    Serial.println(rec.value);
                    break;
                }
            // case MessageLogMode:
            //     {
            //         break;
            //     }
            default:
                Serial.print("Unexpected mode:  ");
                Serial.println(logs[i].mode);
                break;
        }
    } 
}

