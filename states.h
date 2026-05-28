#ifndef STATES_H
#define STATES_H

// State for rcopy
typedef enum rcopyState {
    RCOPY_START_STATE,
    RCOPY_FILENAME_STATE,
    RCOPY_DATA_STATE,
    RCOPY_DONE_STATE
} RCOPY_STATE;

typedef enum serverState {
    SERVER_START_STATE,
    SERVER_FILENAME_STATE,
    SERVER_DATA_STATE,
    SERVER_DONE_STATE
} SERVER_STATE;

#endif
