#include <stdio.h>
#include <stdbool.h>
#include "sig_event.h"
#include <stdatomic.h>

#define BUFFER_SIZE 16 // Use power of 2 for potential bitwise optimizations
typedef enum {
    INV, IDLE, REC
} recorder_states;
typedef enum {
    CLIENT_IDLE, CLIENT_REC, CLIENT_STOP
} client_states;

typedef struct {
    uint8_t *buffer;
    uint8_t *buffer_ptrs[BUFFER_SIZE];
    size_t sz_one;
    int head; // Write index
    int tail; // Read index
    sig_event_t *signal;
    pthread_mutex_t lock;
    atomic_int recorder_state;
    atomic_int client_state;
} fifo_buffer_t;

// Function Prototypes
void fifo_init(fifo_buffer_t **f, size_t sz_one_buff);
bool fifo_is_empty(fifo_buffer_t *f);
bool fifo_is_full(fifo_buffer_t *f);
bool fifo_write(fifo_buffer_t *f, uint8_t *data);
bool fifo_read(fifo_buffer_t *f, uint8_t **data);
recorder_states fifo_recorder_state(fifo_buffer_t *f);
client_states fifo_client_state(fifo_buffer_t *f);
void fifo_recorder_state_set(fifo_buffer_t *f, recorder_states state);
void fifo_client_state_set(fifo_buffer_t *f, client_states state);

// Function Definitions
#ifdef FIFO_IMPLEMENTATION
/**
 * Initialize the FIFO buffer.
 */
void fifo_init(fifo_buffer_t **f, size_t sz_one_buff) {
    bool success;
    *f = malloc(sizeof(fifo_buffer_t));
    success = (NULL != f);
    if (success)
    {
        (*f)->sz_one = sz_one_buff;
        (*f)->buffer = malloc(sz_one_buff * BUFFER_SIZE);
        success = (NULL != (*f)->buffer);
    }
    if (success)
    {
        for (int i = 0; i < BUFFER_SIZE; ++i) 
        {
            (*f)->buffer_ptrs[i] = &(*f)->buffer[i * sz_one_buff];
        }
        (*f)->head = 0;
        (*f)->tail = 0;
    }
    if (success)
    {
        sig_event_init(&(*f)->signal);
        success = (NULL != (*f)->signal);
        pthread_mutex_init(&(*f)->lock, NULL);
        atomic_store_explicit(&(*f)->recorder_state, INV, memory_order_release);
        atomic_store_explicit(&(*f)->client_state, INV, memory_order_release);
    }
}

/**
 * Check if the buffer is empty.
 * Empty occurs when head and tail pointers are equal.
 */
bool fifo_is_empty(fifo_buffer_t *f) {
    return f->head == f->tail;
}

/**
 * Check if the buffer is full.
 * A common method to differentiate full from empty is to "waste" one slot.
 */
bool fifo_is_full(fifo_buffer_t *f) {
    // Buffer is full if the next head position is the current tail position
    return (f->head + 1) % BUFFER_SIZE == f->tail;
}

/**
 * Write data to the buffer.
 * Returns false if the buffer is full, true on success.
 */
bool fifo_write(fifo_buffer_t *f, uint8_t *data) {
    bool rc = true;
    pthread_mutex_lock(&f->lock); 
    if (fifo_is_full(f)) {
        rc = false;
        goto fifo_write_end; // Handle error: buffer is full
    }
    f->buffer_ptrs[f->head] = memcpy((uint8_t *)f->buffer_ptrs[f->head], data, f->sz_one);
    f->head = (f->head + 1) % BUFFER_SIZE; // Wrap around
    sig_event_trigger(f->signal);
fifo_write_end:
    pthread_mutex_unlock(&f->lock);
    return rc;
}

/**
 * Read data from the buffer.
 * Returns false if the buffer is empty, true on success.
 */
bool fifo_read(fifo_buffer_t *f, uint8_t **data) 
{
    bool rc = true;
    sig_event_wait(f->signal);
    pthread_mutex_lock(&f->lock);
    if (fifo_is_empty(f)) {
        sig_event_reset(f->signal);
        rc = false; // Handle error: buffer is empty
        goto fifo_read_end;
    }
    *data = f->buffer_ptrs[f->tail];
    f->tail = (f->tail + 1) % BUFFER_SIZE; // Wrap around
fifo_read_end:
    pthread_mutex_unlock(&f->lock);
    return rc;
}


/**
  * Destroy the buffer.
  */
void fifo_destroy(fifo_buffer_t *f) {
    free(f->buffer);
    sig_event_destroy(f->signal);
    pthread_mutex_destroy(&f->lock);
    free(f->signal);
    free(f);
    f = NULL;
}

client_states fifo_client_state(fifo_buffer_t *f)
{
    return atomic_load_explicit(&f->client_state, memory_order_acquire); 
}
void fifo_client_state_set(fifo_buffer_t *f, client_states state)
{
    atomic_store_explicit(&f->client_state, state, memory_order_release);
}
recorder_states fifo_recorder_state(fifo_buffer_t *f)
{
    return atomic_load_explicit(&f->recorder_state, memory_order_acquire);
}
void fifo_recorder_state_set(fifo_buffer_t *f, recorder_states state)
{
    atomic_store_explicit(&f->recorder_state, state, memory_order_release);
}



#endif // FIFO_IMPLEMENTATION
