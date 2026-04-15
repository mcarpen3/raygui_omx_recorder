#include <stdio.h>
#include <stdbool.h>
#include "sig_event.h"
#include <stdatomic.h>
#include <string.h>

#define BUFFER_SIZE 16 // Use power of 2 for potential bitwise optimizations

typedef enum {
    INV, IDLE, REC, EXIT, REC_PLAY
} recorder_states;

typedef enum {
    CLIENT_INV, CLIENT_IDLE, CLIENT_REC, CLIENT_STOP, CLIENT_EXIT, CLIENT_PLAY
} client_states;

typedef enum {
    PLAYER_INV, PLAYER_PLAY, PLAYER_PAUSE, PLAYER_RW, PLAYER_FF
} player_states;

typedef struct {
    uint8_t *buffer;
    uint8_t *buffer_ptrs[BUFFER_SIZE];
    int64_t timestamps[BUFFER_SIZE];
    size_t sz_one;
    int head; // Write index
    int tail; // Read index
    sig_event_t *signal;
    sig_event_t *player_filename_signal;
    pthread_mutex_t lock;
    atomic_int recorder_state;
    atomic_int client_state;
    atomic_int player_state;
    char *player_filename;
} fifo_buffer_t;

// Function Prototypes
void fifo_init(fifo_buffer_t **f, size_t sz_one_buff);
bool fifo_is_empty(fifo_buffer_t *f);
bool fifo_is_full(fifo_buffer_t *f);
bool fifo_write(fifo_buffer_t *f, uint8_t *data, int64_t frame_ts);
bool fifo_read(fifo_buffer_t *f, uint8_t **data, int64_t *frame_ts);
recorder_states fifo_recorder_state(fifo_buffer_t *f);
client_states fifo_client_state(fifo_buffer_t *f);
player_states fifo_player_state(fifo_buffer_t *f);
void fifo_recorder_state_set(fifo_buffer_t *f, recorder_states state);
void fifo_client_state_set(fifo_buffer_t *f, client_states state);
void fifo_player_state_set(fifo_buffer_t *f, player_states state);
void fifo_player_set_filename(fifo_buffer_t *f, char *file_name);
void fifo_player_get_filename(fifo_buffer_t *f, char **file_name);

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
        atomic_store_explicit(&(*f)->client_state, CLIENT_INV, memory_order_release);
        atomic_store_explicit(&(*f)->player_state, PLAYER_INV, memory_order_release);
    }
    if (success)
    {
        sig_event_init(&(*f)->player_filename_signal);
        success = (NULL != (*f)->player_filename_signal);
    }
    (*f)->player_filename = NULL;
}

/**
 * Check if the buffer is empty.
 * Empty occurs when head and tail pointers are equal.
 */
bool fifo_is_empty(fifo_buffer_t *f) {
    return f->tail == f->head;
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
bool fifo_write(fifo_buffer_t *f, uint8_t *data, int64_t ts) {
    bool rc = true;
    pthread_mutex_lock(&f->lock); 
    if (fifo_is_full(f)) {
        // rc = false;
        // [. . . H T . . .] // overwrite
        f->tail = (f->tail + 1) % BUFFER_SIZE;
        // goto fifo_write_end; // Handle error: buffer is full
    }
    f->buffer_ptrs[f->head] = memcpy((uint8_t *)f->buffer_ptrs[f->head], data, f->sz_one);
    f->timestamps[f->head] = ts;
    f->head = (f->head + 1) % BUFFER_SIZE; // Wrap around
    sig_event_trigger(f->signal);
// fifo_write_end:
    pthread_mutex_unlock(&f->lock);
    return rc;
}

/**
 * Read data from the buffer.
 * Returns false if the buffer is empty, true on success.
 */
bool fifo_read(fifo_buffer_t *f, uint8_t **data, int64_t *frame_ts) 
{
    bool rc = true, timed_out = false;
    //sig_event_wait(f->signal);
    timed_out = sig_event_wait_timeout(f->signal, 16);
    if (timed_out) return false;
    pthread_mutex_lock(&f->lock);
    if (fifo_is_empty(f)) {
        sig_event_reset(f->signal);
        rc = false; // Handle error: buffer is empty
        goto fifo_read_end;
    }
    *data = f->buffer_ptrs[f->tail];
    *frame_ts = f->timestamps[f->tail];
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
    sig_event_destroy(f->player_filename_signal);
    pthread_mutex_destroy(&f->lock);
    free(f->signal);
    free(f->player_filename_signal);
    if (f->player_filename)
    {
        free(f->player_filename);
    }
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
player_states fifo_player_state(fifo_buffer_t *f)
{
    return atomic_load_explicit(&f->player_state, memory_order_acquire);
}
void fifo_player_state_set(fifo_buffer_t *f, player_states state)
{
    atomic_store_explicit(&f->player_state, state, memory_order_release);
}

void fifo_player_set_filename(fifo_buffer_t *f, char *file_name)
{
    if (f->player_filename)
    {
        free(f->player_filename);
        f->player_filename = NULL;
    }
    f->player_filename = strdup(file_name);
    sig_event_trigger(f->player_filename_signal);
}

void fifo_player_get_filename(fifo_buffer_t *f, char **file_name)
{
    sig_event_wait(f->player_filename_signal);
    *file_name = strdup(f->player_filename);
    sig_event_reset(f->player_filename_signal);
}
#endif // FIFO_IMPLEMENTATION
