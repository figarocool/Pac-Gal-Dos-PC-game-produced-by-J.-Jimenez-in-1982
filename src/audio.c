/* C implementation of BASIC PLAY behavior and PC-speaker PIT synthesis.
   Constants and rounding follow the analyzed runtime. Structured effect
   scores and captured DOS results are compared by the audio tests. */
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
/* Shared articulation rules for parsed PLAY commands and structured scores. */
static int articulate(const BasicMusic *m,BasicScore *s,unsigned hz,unsigned ticks) {
 if(!ticks)return 0;
 if(!hz||m->articulation==1)return basic_sound(s,hz,ticks);
 unsigned sounding=(ticks*(m->articulation==2?3u:7u))>>m->articulation;
 unsigned rest=ticks>>m->articulation;
 if(!sounding)sounding=1;
 if(!rest)rest=1;
 if(basic_sound(s,hz,sounding)||basic_sound(s,0,rest))return -1;
 return 0;
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
   if(articulate(m,s,hz,ticks))return -1;
  } else return -1;
 }
 return 0;
}
/* Structured melodies replace the historical BASIC PLAY strings.
   Notes are semitone offsets within an octave; a negative pitch is a pause.
   Length zero and octave -1 retain the preceding setting. This deliberately
   preserves the original sound, including state inherited between effects. */
enum Pitch { REST=-1,C=0,CS,D,DS,E,F,FS,G,GS,A,AS,B };
typedef struct {signed char pitch,octave;unsigned char length;} EffectNote;
static int effect_note(BasicMusic *m,BasicScore *s,int pitch) {
 unsigned hz=0;
 if(pitch!=REST) {
  unsigned shift=6u-m->octave;
  hz=shift?(note_hz[pitch]+(1u<<(shift-1)))>>shift:note_hz[pitch];
 }
 return articulate(m,s,hz,139776u/(m->tempo*m->length));
}
static int melody(BasicMusic *m,BasicScore *s,const EffectNote *notes,unsigned count) {
 for(unsigned i=0;i<count;i++) {
  if(notes[i].octave>=0)m->octave=(unsigned)notes[i].octave;
  if(notes[i].length)m->length=notes[i].length;
  if(effect_note(m,s,notes[i].pitch))return -1;
 }
 return 0;
}
int basic_effect(BasicMusic *m,BasicScore *s,int event) {
 static const EffectNote intro[]={
  {B,-1,0},{B,-1,0},{B,-1,0},{C,-1,16},{E,-1,0},{C,-1,0},
  {B,-1,8},{B,-1,0},{REST,-1,0},{B,-1,0},{B,-1,0},{B,-1,0},
  {C,-1,16},{E,-1,8},{D,-1,0},{C,-1,0},{C,-1,0}
 };
 static const EffectNote dot[]={
  {A,-1,0},{F,-1,0},{G,-1,0},{A,-1,0},{D,4,0}
 };
 static const EffectNote power[]={
  {A,-1,0},{B,-1,0},{C,-1,0},{E,-1,0},
  {A,-1,0},{B,-1,0},{C,-1,0},{E,-1,0},
  {E,-1,0},{B,-1,0},{C,-1,0},{E,-1,0},
  {A,-1,0},{B,-1,0},{C,-1,0},{E,-1,0},
  {A,-1,0},{G,-1,0},{A,-1,0},{A,-1,0}
 };
 static const EffectNote death[]={
  {F,-1,0},{E,-1,0},{G,-1,0},{A,3,0},{B,-1,0},
  {C,-1,0},{D,-1,0},{E,-1,0},{F,-1,0},{G,-1,0},
  {FS,0,1},{G,-1,0}
 };
 switch(event) {
  case SOUND_INTRO:
   m->background=1;m->tempo=190;m->octave=2;m->length=8;
   return melody(m,s,intro,sizeof intro/sizeof *intro);
  case SOUND_DOT:
   m->tempo=255;m->background=1;m->length=64;m->octave=1;
   return melody(m,s,dot,sizeof dot/sizeof *dot);
  case SOUND_POWER:
   m->background=1;m->length=64;
   return melody(m,s,power,sizeof power/sizeof *power);
  case SOUND_DEATH:
   m->background=1;m->length=8;m->tempo=255;m->octave=4;
   return melody(m,s,death,sizeof death/sizeof *death);
  case SOUND_EAT:
   m->background=1;
   for(unsigned octave=2;octave<=4;octave++) {
    m->octave=octave;m->length=octave==2?24:octave==3?32:64;
    for(int pitch=C;pitch<=B;pitch++)if(effect_note(m,s,pitch))return -1;
   }
   return 0;
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
