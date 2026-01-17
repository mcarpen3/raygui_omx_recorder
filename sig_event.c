/*
 * Includes
 */
#include <errno.h>

#include "sig_event.h"

/*
 * Types and Defines
 */

/*
 * Constants
 */

/*
 * Global Variables
 */

/*
 * Function Prototypes
 */

/*
 * Initializes a signal event
 */
void sig_event_init( sig_event_t ** se )
{
    *se = malloc(sizeof(sig_event_t));
    (*se)->signal_triggered = false;
    pthread_mutex_init( &(*se)->signal_mutex, NULL );
    pthread_cond_init( &(*se)->signal_cond, NULL );
}

/*
 * Destroys A Signal Event
 */
void sig_event_destroy( sig_event_t * se )
{
    pthread_mutex_destroy( &se->signal_mutex );
    pthread_cond_destroy( &se->signal_cond );
}

/*
 * Triggers A Signal Event
 *  This causes any thread that is waiting on
 *  the event to wake up.
 */
void sig_event_trigger( sig_event_t * se )
{
    pthread_mutex_lock( &se->signal_mutex );
    se->signal_triggered = true;
    pthread_cond_broadcast( &se->signal_cond );
    pthread_mutex_unlock( &se->signal_mutex );
}

/*
 * Reset A Signal Event
 *  Resets a signal event that has already
 *  been triggered.
 */
void sig_event_reset( sig_event_t * se )
{
    pthread_mutex_lock( &se->signal_mutex );
    se->signal_triggered = false;
    pthread_mutex_unlock( &se->signal_mutex );
}

/*
 * Wait On A Signal Event
 *  A thread will block until the signal
 *  condition indicates that it has been
 *  triggered. Note, due to spurious
 *  returns from pthread_cond_wait, we
 *  must use an external variable to
 *  indicate the state of the signal.
 *  This signal state is called
 *  signal_triggered.
 */
void sig_event_wait( sig_event_t * se )
{
    pthread_mutex_lock( &se->signal_mutex );
    while( false == se->signal_triggered )
    {
        pthread_cond_wait( &se->signal_cond, &se->signal_mutex );
    }
    pthread_mutex_unlock( &se->signal_mutex );
}

/*
 * Wait On A Signal Event With Timeout
 *  A thread will block until the signal
 *  condition indicates that it has been
 *  triggered or the timeout has expired.
 *  Note, due to spurious returns from
 *  pthread_cond_wait, we must use an
 *  external variable to indicate the
 *  state of the signal.
 *  This signal state is called
 *  signal_triggered.
 */
bool sig_event_wait_timeout( sig_event_t * se, uint32_t timeout_ms )
{
    /*
     * Local Variables
     */
    int                 rc;
    struct timespec     timeout;
    bool                timed_out;

    /*
     * Initialize Variables
     */
    timed_out           = false;

    /*
     * Calculate Timeout
     */
    // clock_gettime(CLOCK_REALTIME, &timeout);
    timespec_get(&timeout, TIME_UTC);

    timeout.tv_sec      += timeout_ms / 1000;
    timeout.tv_nsec     += timeout_ms % 1000;

    if( timeout.tv_nsec >= 1000000000L )
    {
        timeout.tv_sec  += 1;
        timeout.tv_nsec -= 1000000000L;
    }

    /*
     * Wait!
     */
    pthread_mutex_lock( &se->signal_mutex );
    while( false == se->signal_triggered && false == timed_out )
    {
        rc = pthread_cond_timedwait( &se->signal_cond, &se->signal_mutex, &timeout );
        timed_out = ( ETIMEDOUT == rc );
    }
    pthread_mutex_unlock( &se->signal_mutex );

    return timed_out;
}
