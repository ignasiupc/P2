#ifndef _VAD_H
#define _VAD_H
#include <stdio.h>

/* TODO: add the needed states */
typedef enum {ST_UNDEF=0, ST_SILENCE, ST_VOICE, ST_INIT} VAD_STATE;

/* Return a string label associated to each state */
const char *state2str(VAD_STATE st);

/* TODO: add the variables needed to control the VAD 
   (counts, thresholds, etc.) */

typedef struct {
  VAD_STATE state;
  float sampling_rate;
  unsigned int frame_length;
  float last_feature; /* for debuggin purposes */
  float p0, p1; /* thresholds */
  /* Variables para estimar el ruido adaptativamente */
  float noise_sum;     /* suma de potencias de frames en fase INIT */
  unsigned int init_count; /* número de frames usados para estimar el ruido */
  float noise_level;   /* nivel de ruido medio (en dB) */
  float k_voice;       /* umbral para detectar voz: noise_level + (10 * α) */
  float k_silence;     /* umbral para confirmar silencio: noise_level + (3 * α) */
  float noise_zcr;     /* nivel de zcr medio */   
  float noise_zcr_sum;
  float max_power;
  float min_power;
  float max_power_real;
  float min_power_real; 

  unsigned int count_voice;   /* contador de frames que indican voz */
  unsigned int count_silence; /* contador de frames que indican silencio */

  unsigned int voice_segment_count;    // Número de segmentos de voz detectados
  unsigned int total_voice_frames;     // Total de frames clasificados como voz
  unsigned int max_silence_in_voice;   // Máxima duración de silencio encontrada en voz
  unsigned int adaptive_hangover;      // Valor adaptativo de hangover

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
