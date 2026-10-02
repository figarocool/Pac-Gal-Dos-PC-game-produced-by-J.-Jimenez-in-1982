#ifndef PACGAL_SPEAKER_H
#define PACGAL_SPEAKER_H
#include "audio.h"
#define SPEAKER_QUEUE 2048u
typedef struct {unsigned hz,samples,ticks;} SpeakerPacket;
typedef struct {float b0,b1,b2,a1,a2,z1,z2;} SpeakerFilter;
typedef struct {
 SpeakerPacket queue[SPEAKER_QUEUE];unsigned head,tail,rate,remaining;
 uint64_t phase,period,high;
 float volume;
 int pc_model,gate;
 uint64_t pending_period,pending_high;
 uint64_t irq_until,irq_period;
 float dc_input,dc_output;
 SpeakerFilter filter[4];
} Speaker;
void speaker_init(Speaker *s,unsigned rate,float volume);
/* Native PC-speaker gate/PIT and small-speaker response. Init before use. */
void speaker_pc_init(Speaker *s,unsigned rate,float volume);
/* DOS-video reference response; same PC timer/gate model. */
void speaker_video_init(Speaker *s,unsigned rate,float volume);
/* Caller synchronizes producer/consumer, e.g. SDL_LockAudioDevice. */
int speaker_enqueue(Speaker *s,const BasicScore *score);
void speaker_float(Speaker *s,float *samples,size_t count);
void speaker_s16(Speaker *s,int16_t *samples,size_t frames,unsigned channels);
#endif
