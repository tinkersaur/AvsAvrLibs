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

/** Log a checkpoint. */
void checkpoint(uint32_t timestamp, int16_t line_number); 

/** Log a message. */
void message(char * mssg); 

/** Trace a value. */
void trace(char * key, int value);

/** Other logging forms that could be added later.
 */

/** Check if the logging queue is full */
bool logs_full();

/** Report all the current logs through a serial port. */
void report_logs();

#endif
