#ifndef SIG_EVENT_H
#define SIG_EVENT_H

/*
 * Includes
 */
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

/*
 * Types and Defines
 */
typedef struct
    {
    pthread_cond_t  signal_cond;        // Condition used by signalMutex to wait waiting threads
    pthread_mutex_t signal_mutex;       // Mutex that protects signalCond and SignalTriggered
    bool            signal_triggered;   // Indicates state of sig_event_t
    } sig_event_t;

/*
* Constants
*/

/*
 * Global Variables
 */

/*
 * Function Prototypes
 */
void sig_event_init( sig_event_t ** se );
void sig_event_destroy( sig_event_t * se );
void sig_event_trigger( sig_event_t * se );
void sig_event_reset( sig_event_t * se );
void sig_event_wait( sig_event_t * se );
bool sig_event_wait_timeout( sig_event_t * se, uint32_t timeout_ms );

#endif //SIG_EVENT_H
