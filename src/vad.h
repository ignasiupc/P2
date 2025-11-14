#ifndef _VAD_H
#define _VAD_H
#include <stdio.h>

/* States for Voice Activity Detection Finite State Automaton:
   - ST_UNDEF: Undefined state (used when not enough information)
   - ST_SILENCE: Confirmed silence segment
   - ST_VOICE: Confirmed voice segment
   - ST_INIT: Initial state (before first frame processing)
   - ST_MAYBE_VOICE: Transitional state when power increases (silence → voice)
   - ST_MAYBE_SILENCE: Transitional state when power decreases (voice → silence)
*/
typedef enum {
  ST_UNDEF=0, 
  ST_SILENCE, 
  ST_VOICE, 
  ST_INIT,
  ST_MAYBE_VOICE,
  ST_MAYBE_SILENCE
} VAD_STATE;

/* Return a string label associated to each state */
const char *state2str(VAD_STATE st);

/* Control variables for VAD state machine:
   - state: current state of the FSM
   - sampling_rate: audio sampling rate
   - frame_length: number of samples per frame
   - last_feature: last computed feature value (for debugging)
   - p0, p1: power thresholds (silence/voice boundaries)
   - frame_counter: counts consecutive frames in transitional states
   - min_duration: minimum number of frames to confirm state change
*/

typedef struct {
  VAD_STATE state;
  float sampling_rate;
  unsigned int frame_length;
  float last_feature; /* for debuggin purposes */
  float p0, p1; /* thresholds */
  unsigned int frame_counter; /* counter for state transitions */
  unsigned int min_duration; /* minimum frames to confirm state change */
} VAD_DATA;

/* Call this function before using VAD: 
   It should return allocated and initialized values of vad_data

   sampling_rate: ... the sampling rate */
VAD_DATA *vad_open(float sampling_rate);

/* vad works frame by frame.
   This function returns the frame size so that the program knows how
   many samples have to be provided */
unsigned int vad_frame_size(VAD_DATA *);

/* Main function. For each 'time', compute the new state 
   It returns:
    ST_UNDEF   (0) : undefined; it needs more frames to take decission
    ST_SILENCE (1) : silence
    ST_VOICE   (2) : voice

    x: input frame
       It is assumed the length is frame_length */
VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1);

/* Free memory
   Returns the state of the last (undecided) states. */
VAD_STATE vad_close(VAD_DATA *vad_data);

/* Print actual state of vad, for debug purposes */
void vad_show_state(const VAD_DATA *, FILE *);

#endif
