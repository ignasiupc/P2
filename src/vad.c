#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"

#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
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

Features compute_features(const float *x, int N, int fm) {
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
  feat.zcr = compute_zcr(x, N, fm);
  feat.am = compute_am(x, N);

  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  /* Inicialización de parámetros adaptativos */
  vad_data->noise_sum = 0.0;
  vad_data->noise_zcr_sum = 0.0;  // Nuevo campo
  vad_data->init_count = 0;
  vad_data->noise_level = -100.0;  /* Valor inicial muy bajo */
  vad_data->k_voice = -40.0;  
  vad_data->k_silence = -50.0;
  
  vad_data->last_feature = 0.0;
  vad_data->p0 = 5;  /* Valor por defecto de α, ya se cambia luego */
  vad_data->count_voice = 0;
  vad_data->count_silence = 0;  // Inicialización del hangover
  vad_data->voice_segment_count = 0;
  vad_data->total_voice_frames = 0;
  vad_data->max_silence_in_voice = 0;
  vad_data->adaptive_hangover = 5;  // Valor inicial
    // In vad_open function
  vad_data->max_power = 0.0;       // Start with reasonable values
  vad_data->min_power = -100.0; 
  vad_data->max_power_real = -60.0;       // Start with reasonable values
  vad_data->min_power_real = -30.0; 
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length, vad_data->sampling_rate);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  // In vad function, after computing features
  // Update min/max power values (with protection against extreme outliers)
  if (f.p < vad_data->min_power) vad_data->min_power = f.p;
  if (f.p > vad_data->max_power) vad_data->max_power = f.p;

  // Calculate normalized power
  float f_norm = 0.0;
  if (vad_data->max_power > vad_data->min_power) {
      f_norm = (f.p - vad_data->min_power) / (vad_data->max_power - vad_data->min_power);
  }
  if (f.p < vad_data->min_power_real) vad_data->min_power_real = f.p;
  if (f.p > vad_data->max_power_real) vad_data->max_power_real = f.p;

  // Calculate normalized power
  float f_norm_real = 0.0;
  if (vad_data->max_power_real > vad_data->min_power_real) {
      f_norm_real = (f.p - vad_data->min_power_real) / (vad_data->max_power_real - vad_data->min_power_real);
  }

  switch (vad_data->state) {
  case ST_INIT:
    vad_data->p0 = f.p;
    vad_data->p1 = vad_data->p0 + alpha1;
    vad_data->state = ST_SILENCE;
    break;

  case ST_SILENCE:
    if (f.p > vad_data->p1){
      vad_data->state = ST_VOICE;
    }
      
    break;

  case ST_VOICE:
    if (f.p < vad_data->p0){
      vad_data->state = ST_SILENCE;
    }
    break;

  case ST_UNDEF:
    /* TODO: Implement your own logic for the undefined state */
    if (f.p > vad_data->p1) {
      vad_data->state = ST_VOICE;
    } else if (f.p < vad_data->p0) {
      vad_data->state = ST_SILENCE;
    }
    
    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE)
    return vad_data->state;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
