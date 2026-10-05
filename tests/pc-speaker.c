/* Speaker behaviour checks beyond the raw PIT parser fixtures. */
#include "speaker.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static double rms(const float *x,size_t n) {
 double e=0;for(size_t i=0;i<n;i++)e+=(double)x[i]*x[i];return sqrt(e/n);
}
static double gain(unsigned rate,unsigned hz) {
 Speaker *s=malloc(sizeof *s);assert(s);speaker_pc_init(s,rate,.15f);
 BasicScore score={0};assert(!basic_sound(&score,hz,1000));assert(!speaker_enqueue(s,&score));
 float *x=malloc(rate*sizeof *x);assert(x);speaker_float(s,x,rate);
 double result=rms(x+rate/2,rate/2);free(x);free(s);return result;
}
int main(void) {
 for(unsigned rate=44100;rate<=48000;rate+=3900) {
  size_t n=rate*2;float *a=calloc(n,sizeof *a),*b=calloc(n,sizeof *b);assert(a&&b);
  Speaker *whole=malloc(sizeof *whole),*chunks=malloc(sizeof *chunks);assert(whole&&chunks);
  for(int profile=0;profile<2;profile++)for(int effect=1;effect<=7;effect++) {
   void (*init)(Speaker*,unsigned,float)=profile?speaker_video_init:speaker_pc_init;
   init(whole,rate,.15f);init(chunks,rate,.15f);
   BasicMusic music;BasicScore score={0};basic_music_init(&music);assert(!basic_effect(&music,&score,effect));
   size_t len=basic_samples(&score,rate)+(profile?rate:rate/4);
   a=realloc(a,len*sizeof *a);b=realloc(b,len*sizeof *b);assert(a&&b);
   assert(!speaker_enqueue(whole,&score));assert(!speaker_enqueue(chunks,&score));
   speaker_float(whole,a,len);
   for(size_t i=0;i<len;) {size_t k=len-i<137?len-i:137;speaker_float(chunks,b+i,k);i+=k;}
   assert(!memcmp(a,b,len*sizeof *a));
   for(size_t i=0;i<len;i++)assert(isfinite(a[i])&&fabsf(a[i])<1.f);
   assert(rms(a+len-rate/100,rate/100)<.000001);
   if(effect==SOUND_FLOOR) {
    size_t tone=basic_samples(&score,rate)+rate/16;
    /* Speaker has a decaying response after its electrical gate is low. */
    assert(rms(a+tone,rate/100)>.001);
    /* Disabled input stays silent after settling, including stereo output. */
    int16_t stereo[512];speaker_s16(whole,stereo,256,2);
    for(unsigned i=0;i<256;i++){assert(stereo[2*i]==stereo[2*i+1]);assert(abs(stereo[2*i])<=1);}
   }
  }
  speaker_pc_init(whole,rate,.15f);float idle[256];speaker_float(whole,idle,256);
  whole->irq_until=(uint64_t)BASIC_SOUND_TICK*rate;
  for(unsigned i=0;i<256;i++)assert(idle[i]==0.f);
  /* A count write is deferred to an edge, retaining the current phase. */
  BasicScore score={0};basic_sound(&score,150,3);basic_sound(&score,4000,6);
  assert(!speaker_enqueue(whole,&score));speaker_float(whole,idle,1);
  uint64_t old_period=whole->period;size_t tone=3ull*BASIC_SOUND_TICK*rate/BASIC_PIT_HZ;
  float *advance=malloc((tone+2)*sizeof *advance);assert(advance);speaker_float(whole,advance,tone+1);
  assert(whole->period==old_period&&whole->pending_period!=0&&whole->phase!=BASIC_PIT_HZ);
  speaker_float(whole,advance,rate/500);assert(whole->period==(uint64_t)(BASIC_PIT_HZ/4000)*rate);
  free(advance);
  /* DOS PIT0 retains the old scheduled IRQ when BASIC changes its count. */
  for(unsigned first=2048;first<=65536;first+=63488) {
   speaker_video_init(whole,rate,.15f);whole->irq_until=(uint64_t)first*rate;
   BasicScore impulse={0};basic_sound(&impulse,150,3);speaker_enqueue(whole,&impulse);
   size_t active=0;do {speaker_float(whole,idle,1);active++;}while(whole->remaining);
   size_t expected=((uint64_t)(first+2*2048)*rate+BASIC_PIT_HZ-1)/BASIC_PIT_HZ;
   assert(active==expected);
  }
  double middle=gain(rate,1000),low=gain(rate,50),high=gain(rate,10000);
  assert(low<middle*.5&&high<middle*.20);
  printf("OK: PC-speaker at %u Hz: all effects, callback invariance, gate transient, count reload, decay, stereo; RMS 50/1000/10000 Hz: %.5f/%.5f/%.5f.\n",rate,low,middle,high);
  free(a);free(b);free(whole);free(chunks);
 }
}
