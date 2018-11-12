#ifndef AvrLog_h

#include<stdint.h>

#define MaxNumLogs 15

static const uint8_t EmptyLogMode = 0;
static const uint8_t ErroLogMode = 1;
static const uint8_t IntValueLogMode = 2;
static const uint8_t CheckpointLogMode = 3;
static const uint8_t MessageLogMode = 4;

struct IntValueRec{
    char key[3];
    int value;
};

struct CheckpointRec{
    int16_t line;
    uint32_t timestamp;
};

struct MessageRec{
    char msg[7];
};


struct LogRecord{
    uint8_t mode;
    union{
        CheckpointRec checkpoint;
        IntValueRec int_value;
    } rec;
};


extern LogRecord logs[MaxNumLogs];
extern uint8_t current_log;


void checkpoint(uint32_t timestamp, int16_t line_number); 
void message(char * mssg); 
void trace(char * key, int value);

bool logs_full();

/** Other logging forms that could be added later: 
void add_log(char * three_char_label, int value);
void add_log(unsigned long timestamp, char * six_char_msg); 
*/

void report_logs();

#endif
