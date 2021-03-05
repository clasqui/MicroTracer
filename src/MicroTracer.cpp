
#include"MicroTracer.h"
#include<stdlib.h>
#include<HardwareSerial.h>
#include<Arduino.h>

mt_event_t events[EVENT_BUFF_SIZE];
char tracing = 0;
int n_event = 0;

struct Event_definition *definitions;
struct Event_definition *last_event_definition;

mt_event_type_t flush_type = 40;

volatile int isr_timer1_overflow_count;


/**
 * @name micros_timer1_ctc
 * Llegeix el temps d'execució del TIMER1 quan està en mode CTC, a l'arquitectura ATmega
 * Aquesta funció requereix que la variable `isr_timer1_overflow_count` s'incrementi per la rutina de 
 * servei de la interrupció del TIMER1.  S'utilitza per llegir el temps en microsegons per les funcions de traceig d'events,
 * en el cas que la macro MT_USE_TIMER1 estigui definida en temps de compilació.
 * 
 * @return Temps en microsegons
 * */
unsigned long micros_timer1_ctc() {
    return 4*(OCR1A*isr_timer1_overflow_count+TCNT1)/(F_CPU / 1000000);
}


/**
 * @name incrementOverflowTimer1
 * Incrementa la variable utilitzada per guardar el compte de overflows del TIMER1 a l'arquitectura ATmega.
 * 
 **/
void incrementOverflowTimer1() {
    isr_timer1_overflow_count++;
    return;
}


/**
 * @name MicroTracer_Init()
 * Inicialitza el sistema de traceig.
 * Posa les variables inicials, crea event de flushing, i activa el traceig (variable tracing)
 * 
 **/
void MicroTracer_init() {
    tracing = 1;
    isr_timer1_overflow_count = 0;

    definitions = NULL;
    last_event_definition = NULL;
    
    Serial.println(F("[MicroTracer:Debug]MicroTracer initialized"));

    unsigned flush_values = 2;
    char *descriptions[] = {"End", "Begin"};
    MicroTracer_define_event_type(flush_type, "Flushing traces", &flush_values, descriptions);

    //Transmit event definitions
    struct Event_definition *ptr = definitions;
    while (ptr != NULL) { 
        
        char buff[32];

        Serial.println(F("[MicroTracer:PCF]EVENT_TYPE"));
        sprintf(buff, "[MicroTracer:PCF]0 %d %s", ptr->type, ptr->description);
        Serial.println(buff);
        Serial.println();
        Serial.println(F("[MicroTracer:PCF]VALUES"));
        Serial.println(F("[MicroTracer:PCF]0 End"));
        for (int i = 0; i < ptr->nvalues; i++)
        {
            sprintf(buff, "[MicroTracer:PCF]%d %s", i+1, ptr->description_values[i]);
        }
        Serial.println();
        
        ptr = ptr->next; 
    }


}

/**
 * @name MicroTracer_finish()
 * FInalitza el traceig i allibera la memoria utilitzada per les descripcions dels events
 * 
 * */
void MicroTracer_finish() {
    MicroTracer_flushEvents();
    Serial.println(F("[MicroTracer:Debug]MicroTracer stops tracing NOW"));
    tracing = 0;
    struct Event_definition *ptr = definitions;
    while (ptr != NULL) { 
        struct Event_definition *aux = ptr->next;
        free(ptr);
        ptr = aux;
    }
    Serial.end();
}


/**
 * MicroTracer_flushEvents()
 * Envia els events guardats al buffer "events" per Serie.
 * La informació enviada està codificada en el format de traça de Paraver.
 * Veure la documentació a https://tools.bsc.es/doc/1370.pdf
 * Quan acaba, situa l'index de event actual a 0 (variable n_event)
 * 
 * */
void MicroTracer_flushEvents() {
    char buff[256];
    MicroTracer_event(flush_type, 1);

    
    for (int i = 0; i < n_event; i++)
    {
        sprintf(buff, "[MicroTracer:PRV]2:1:1:1:%d:%lu:%d:%d", events[i].thread_id, events[i].timestamp_us, events[i].event_type, events[i].event_value);
        Serial.println(buff);
    }
    

    //sprintf(buff, "[MicroTracer:Debug]Flushed %d events to serial", n_event);
    //Serial.println(buff);
    n_event = 0;

    MicroTracer_event(flush_type, 0);

}

/**
 * MicroTracer_event()
 * Registra un event.
 * @param mt_event_type type El numero de tipus d'event a registrar
 * @param int value el valor de l'event. Si volem finalitzar un event anterior, el posem a 0.
 * 
 * @return -1 En el cas que el buffer estigui ple o el traceig estigui desactivat.
 * @return n_event El número de events guardats al buffer
 * */
int MicroTracer_event(mt_event_type_t type, int value) {
    if(!tracing) return -1;
    if(n_event == EVENT_BUFF_SIZE) {
        return -1;
    }
    
    events[n_event].event_type = type;
    events[n_event].event_value = value;
    events[n_event].thread_id = 1;

    #ifdef MT_USE_TIMER1
    events[n_event].timestamp_us = micros_timer1();
    #else
    events[n_event].timestamp_us = micros();
    #endif

    n_event++;
    return n_event;
}


/**
 * MicroTracer_define_event_type()
 * Defineix un tipus d'event, amb la seva descripció, i els seus possibles valors i corresponents descripcions.
 * Aquesta informació s'incorpora a l'arxiu PCF de la traça, que després es mostra a Paraver per facilitar la feina.
 * */
void MicroTracer_define_event_type(mt_event_type_t type, char *description, unsigned *nvalues, char **description_values) {
    struct Event_definition * nDef = (struct Event_definition *)malloc(sizeof(struct Event_definition));


    strcpy(nDef->description, description);
    nDef->type = type;
    nDef->nvalues = *nvalues;
    nDef->description_values = description_values;
    nDef->next = NULL;

    if(last_event_definition == NULL) {
        last_event_definition = nDef;
        definitions = nDef;
        
    } else {
        last_event_definition->next = nDef;
        last_event_definition = nDef;
    }

}

