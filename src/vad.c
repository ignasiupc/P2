#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"

#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * Labels for VAD states
 * - UNDEF: Undefined (not enough information)
 * - S: Silence (confirmed)
 * - V: Voice (confirmed)
 * - INIT: Initial state
 * - MAYBE_V: Transitional state (maybe voice)
 * - MAYBE_S: Transitional state (maybe silence)
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MAYBE_V", "MAYBE_S"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  /*feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;*/

  feat.p = compute_power(x, N);

  return feat;
}

/* 
 * Initialize VAD data structure
 * - Set initial state to ST_INIT
 * - Calculate frame length based on sampling rate and frame time
 * - Initialize frame counter and minimum duration
 *   min_duration = 2 frames (20ms at 10ms per frame) to avoid spurious single-frame transitions
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->frame_counter = 0;
  vad_data->min_duration = 2; /* 2 frames = 20ms minimum duration to filter glitches */
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * Decide what to do with the last undecided frames
   * - If in a transitional state (MAYBE_VOICE/MAYBE_SILENCE), 
   *   return the previous stable state
   * - ST_MAYBE_VOICE frames are treated as SILENCE (not sustained enough)
   * - ST_MAYBE_SILENCE frames are treated as VOICE (not sustained enough)
   */
  VAD_STATE final_state;
  
  switch (vad_data->state) {
  case ST_MAYBE_VOICE:
    /* Transitional voice not confirmed, treat as silence */
    final_state = ST_SILENCE;
    break;
  case ST_MAYBE_SILENCE:
    /* Transitional silence not confirmed, treat as voice */
    final_state = ST_VOICE;
    break;
  case ST_INIT:
  case ST_UNDEF:
    /* No clear decision, default to silence */
    final_state = ST_SILENCE;
    break;
  default:
    /* Already in a stable state */
    final_state = vad_data->state;
    break;
  }

  free(vad_data);
  return final_state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * Voice Activity Detection using Finite State Automaton
 * 
 * State transitions:
 * - ST_INIT → ST_SILENCE: Initialize thresholds from first frame
 * - ST_SILENCE → ST_MAYBE_VOICE: Power exceeds p1 threshold
 * - ST_MAYBE_VOICE → ST_VOICE: Sustained high power for min_duration frames
 * - ST_MAYBE_VOICE → ST_SILENCE: Power drops back below p1
 * - ST_VOICE → ST_MAYBE_SILENCE: Power drops below p0 threshold  
 * - ST_MAYBE_SILENCE → ST_SILENCE: Sustained low power for min_duration frames
 * - ST_MAYBE_SILENCE → ST_VOICE: Power increases back above p0
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1) {

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature for debugging */

  switch (vad_data->state) {
  case ST_INIT:
    /* Initialize thresholds based on first frame (assumed to be silence) */
    vad_data->p0 = f.p;
    vad_data->p1 = vad_data->p0 + alpha1;
    vad_data->state = ST_SILENCE;
    vad_data->frame_counter = 0;
    break;

  case ST_SILENCE:
    if (f.p > vad_data->p1) {
      /* Power exceeded voice threshold, enter transitional state */
      vad_data->state = ST_MAYBE_VOICE;
      vad_data->frame_counter = 1;
    }
    break;

  case ST_MAYBE_VOICE:
    if (f.p > vad_data->p1) {
      /* Power still high, increment counter */
      vad_data->frame_counter++;
      if (vad_data->frame_counter >= vad_data->min_duration) {
        /* Sustained high power, confirm as voice */
        vad_data->state = ST_VOICE;
        vad_data->frame_counter = 0;
      }
    } else {
      /* Power dropped, return to silence */
      vad_data->state = ST_SILENCE;
      vad_data->frame_counter = 0;
    }
    break;

  case ST_VOICE:
    if (f.p < vad_data->p0) {
      /* Power dropped below silence threshold, enter transitional state */
      vad_data->state = ST_MAYBE_SILENCE;
      vad_data->frame_counter = 1;
    }
    break;

  case ST_MAYBE_SILENCE:
    if (f.p < vad_data->p0) {
      /* Power still low, increment counter */
      vad_data->frame_counter++;
      if (vad_data->frame_counter >= vad_data->min_duration) {
        /* Sustained low power, confirm as silence */
        vad_data->state = ST_SILENCE;
        vad_data->frame_counter = 0;
      }
    } else {
      /* Power increased, return to voice */
      vad_data->state = ST_VOICE;
      vad_data->frame_counter = 0;
    }
    break;

  case ST_UNDEF:
    /* Fallback logic for undefined state */
    if (f.p > vad_data->p1) {
      vad_data->state = ST_MAYBE_VOICE;
      vad_data->frame_counter = 1;
    } else if (f.p < vad_data->p0) {
      vad_data->state = ST_SILENCE;
      vad_data->frame_counter = 0;
    }
    break;
  }

  /* Return only confirmed states (SILENCE or VOICE), 
     transitional states return UNDEF */
  if (vad_data->state == ST_SILENCE || vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
