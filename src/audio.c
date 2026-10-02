/* Translation of the BASIC PLAY parser and PC-speaker PIT tone generator.
   Constants and integer rounding are recovered from the original runtime.
   The independently instrumented DOS parser is the test oracle. */
#include "audio.h"
#include <string.h>
#include <ctype.h>
#include <math.h>
static const unsigned note_hz[]={4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,7459,7902};
void basic_music_init(BasicMusic *m){*m=(BasicMusic){120,4,4,3,0};}
int basic_sound(BasicScore *s,unsigned hz,unsigned ticks) {
 if(!ticks)return 0;
 if(s->count>=BASIC_AUDIO_MAX||hz>65535||ticks>65535)return -1;
 s->tone[s->count++]=(BasicTone){(uint16_t)hz,(uint16_t)ticks};return 0;
}
static unsigned number(const char **p) {
 unsigned n=0;while(isdigit((unsigned char)**p)){n=n*10+(unsigned)(*(*p)++-'0');if(n>65535)return 65536;}return n;
}
int basic_play(BasicMusic *m,BasicScore *s,const char *p) {
 static const int pitch[]={9,11,0,2,4,5,7};
 while(*p) {
  int c=toupper((unsigned char)*p++);
  if(c==' '||c==';')continue;
  if(c=='M') {
   c=toupper((unsigned char)*p++);
   if(c=='B')m->background=1;else if(c=='F')m->background=0;
   else if(c=='L')m->articulation=1;else if(c=='S')m->articulation=2;else if(c=='N')m->articulation=3;else return -1;
  } else if(c=='T'||c=='L'||c=='O') {
   unsigned n=number(&p);
   if(c=='T'){if(n<32||n>255)return -1;m->tempo=n;}
   else if(c=='L'){if(!n||n>64)return -1;m->length=n;}
   else {if(n>6)return -1;m->octave=n;}
  } else if(c=='>'||c=='<') {
   if(c=='>'&&m->octave<6)m->octave++;else if(c=='<'&&m->octave)m->octave--;else return -1;
  } else if((c>='A'&&c<='G')||c=='P'||c=='N') {
   unsigned hz=0,l=m->length;int semitone=0,octave=(int)m->octave;
   if(c=='N') {
    unsigned n=number(&p);if(n>84)return -1;
    if(n){octave=(int)(n-1)/12;semitone=(int)(n-1)%12;c='C';}else c='P';
   } else {
    if(c!='P')semitone=pitch[c-'A'];
    if(*p=='#'||*p=='+'){semitone++;p++;}else if(*p=='-'){semitone--;p++;}
    unsigned n=number(&p);if(n){if(n>64)return -1;l=n;}
   }
   if(c!='P') {
    if(semitone<0){semitone+=12;octave--;}else if(semitone>11){semitone-=12;octave++;}
    if(octave<0||octave>6)return -1;
    unsigned shift=6u-(unsigned)octave;
    hz=shift?(note_hz[semitone]+(1u<<(shift-1)))>>shift:note_hz[semitone];
   }
   unsigned ticks=139776u/(m->tempo*l);
   while(*p=='.'){ticks=ticks*3/2;p++;}
   if(!ticks)continue;
   if(!hz||m->articulation==1){if(basic_sound(s,hz,ticks))return -1;}
   else {
    unsigned sound_ticks=(ticks*(m->articulation==2?3u:7u))>>m->articulation;
    unsigned rest_ticks=ticks>>m->articulation;
    if(!sound_ticks)sound_ticks=1;
    if(!rest_ticks)rest_ticks=1;
    if(basic_sound(s,hz,sound_ticks)||basic_sound(s,0,rest_ticks))return -1;
   }
  } else return -1;
 }
 return 0;
}
int basic_effect(BasicMusic *m,BasicScore *s,int event) {
 switch(event) {
  case SOUND_INTRO:return basic_play(m,s,"mbt190o2l8bbbl16cecl8bbp8bbbl16cl8edcc");
  case SOUND_DOT:return basic_play(m,s,"t255mbl64o1afgao4d");
  case SOUND_POWER:return basic_play(m,s,"mbl64abceabceebceabceagaa");
  case SOUND_DEATH:return basic_play(m,s,"mbl8t255o4fego3abcdefgo0l1g-g");
  case SOUND_EAT:return basic_play(m,s,"mbl24o2cc#dd#eff#gg#aa#bl32o3cc#dd#eff#gg#aa#bl64o4cc#dd#eff#gg#aa#b");
  case SOUND_WALL:return basic_sound(s,4000,6);
  case SOUND_FLOOR:return basic_sound(s,150,3);
  default:return 0;
 }
}
size_t basic_samples(const BasicScore *s,unsigned rate) {
 uint64_t ticks=0;for(unsigned i=0;i<s->count;i++)ticks+=s->tone[i].ticks;
 return (size_t)(ticks*BASIC_SOUND_TICK*rate/BASIC_PIT_HZ);
}
/* Integrate the actual mode-3 PIT pulse over a sample interval. Odd divisors
   have a high half one PIT clock longer than the low half. This avoids
   choosing a modern equal-tempered oscillator instead of the PC hardware. */
static double high_area(double t,unsigned divisor) {
 unsigned high=(divisor+1)/2;
 double periods=floor(t/divisor),remainder=t-periods*divisor;
 return periods*high+(remainder<high?remainder:high);
}
void basic_render(const BasicScore *s,float *samples,unsigned rate,float volume) {
 uint64_t ticks=0;size_t first=0;
 for(unsigned i=0;i<s->count;i++) {
  const BasicTone *t=&s->tone[i];ticks+=t->ticks;
  size_t end=(size_t)(ticks*BASIC_SOUND_TICK*rate/BASIC_PIT_HZ);
  unsigned divisor=t->hz?BASIC_PIT_HZ/t->hz:0;
  double width=(double)BASIC_PIT_HZ/rate;
  for(size_t j=first;j<end;j++) {
   if(!divisor)samples[j]=0;
   else {double a=(j-first)*width,b=a+width;samples[j]=(float)(volume*(2*(high_area(b,divisor)-high_area(a,divisor))/width-1));}
  }
  first=end;
 }
}
