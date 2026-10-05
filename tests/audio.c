#include "audio.h"
#include "audio-fixtures.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static int check(const char *name,const BasicScore *s,const BasicTone *tones,size_t count) {
 if(count!=s->count){fprintf(stderr,"%s: note count C %u DOS %zu\n",name,s->count,count);return 1;}
 for(size_t i=0;i<count;i++)if(s->tone[i].hz!=tones[i].hz||s->tone[i].ticks!=tones[i].ticks){fprintf(stderr,"%s note %zu: C %u/%u DOS %u/%u\n",name,i,s->tone[i].hz,s->tone[i].ticks,tones[i].hz,tones[i].ticks);return 1;}
 return 0;
}
#define CHECK(n,e) do {BasicMusic m;BasicScore s={0};basic_music_init(&m);if(basic_effect(&m,&s,e)||check(#n,&s,oracle_##n,sizeof oracle_##n/sizeof *oracle_##n))return 1;}while(0)
int main(void) {
 CHECK(intro,SOUND_INTRO);CHECK(dot,SOUND_DOT);CHECK(power,SOUND_POWER);CHECK(death,SOUND_DEATH);CHECK(eat,SOUND_EAT);CHECK(wall,SOUND_WALL);CHECK(floor,SOUND_FLOOR);
 BasicMusic m;BasicScore s={0};basic_music_init(&m);
 int seq[]={SOUND_INTRO,SOUND_DOT,SOUND_POWER,SOUND_WALL,SOUND_FLOOR,SOUND_DEATH,SOUND_EAT};
 for(unsigned i=0;i<sizeof seq/sizeof *seq;i++)if(basic_effect(&m,&s,seq[i]))return 1;
 if(check("persistent sequence",&s,oracle_sequence,sizeof oracle_sequence/sizeof *oracle_sequence))return 1;
 size_t n=basic_samples(&s,44100);float *samples=malloc(n*sizeof *samples);if(!samples)return 1;
 basic_render(&s,samples,44100,.15f);
 for(size_t i=0;i<n;i++)if(!isfinite(samples[i])||fabsf(samples[i])>.1501f)return 1;
 free(samples);
 puts("OK: all 7 effects and persistent sequence match DOS frequencies, durations and rests; PIT synthesis valid.");return 0;
}
