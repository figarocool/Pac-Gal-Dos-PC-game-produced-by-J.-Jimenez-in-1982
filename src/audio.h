#ifndef PACGAL_AUDIO_H
#define PACGAL_AUDIO_H
#include <stddef.h>
#include <stdint.h>
#define BASIC_AUDIO_MAX 512
#define BASIC_PIT_HZ 1193180u
#define BASIC_SOUND_TICK 2048u
typedef struct {uint16_t hz,ticks;} BasicTone;
typedef struct {BasicTone tone[BASIC_AUDIO_MAX];unsigned count;} BasicScore;
typedef struct {unsigned tempo,length,octave,articulation;int background;} BasicMusic;
enum {SOUND_DOT=1,SOUND_WALL,SOUND_DEATH,SOUND_EAT,SOUND_POWER,SOUND_FLOOR,SOUND_INTRO};
void basic_music_init(BasicMusic *m);
int basic_play(BasicMusic *m,BasicScore *score,const char *commands);
int basic_sound(BasicScore *score,unsigned hz,unsigned ticks);
int basic_effect(BasicMusic *m,BasicScore *score,int event);
size_t basic_samples(const BasicScore *s,unsigned sample_rate);
void basic_render(const BasicScore *s,float *samples,unsigned sample_rate,float volume);
#endif
