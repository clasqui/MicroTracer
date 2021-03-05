#ifndef MicroTracer_H
#define MicroTracer_H

#ifndef EVENT_BUFF_SIZE
#define EVENT_BUFF_SIZE 128
#endif

typedef int mt_event_type_t;

typedef struct {
    mt_event_type_t event_type;
    int event_value;
    unsigned long timestamp_us;
    unsigned int thread_id;
} mt_event_t;

struct Event_definition { 
    mt_event_type_t type;
    char description[128];
    unsigned nvalues;
    char **description_values;
    struct Event_definition* next;
};

void MicroTracer_init(void);
void MicroTracer_finish(void);
void MicroTracer_flushEvents(void);
int  MicroTracer_event(mt_event_type_t type, int value);
void MicroTracer_define_event_type(mt_event_type_t type, char *description, unsigned *nvalues, char **description_values);

void incrementOverflowTimer1();

#endif