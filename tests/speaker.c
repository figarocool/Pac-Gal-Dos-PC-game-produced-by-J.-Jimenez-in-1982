#include "speaker.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(void) {
 double max_error=0;clock_t start=clock();
 for(unsigned rate=44100;rate<=48000;rate+=3900) {
  BasicMusic music;basic_music_init(&music);
  for(int event=1;event<=7;event++) {
   BasicScore score={0};assert(!basic_effect(&music,&score,event));size_t n=basic_samples(&score,rate);
   float *reference=malloc(n*sizeof *reference),*actual=malloc(n*sizeof *actual);assert(reference&&actual);
   basic_render(&score,reference,rate,.15f);Speaker *s=malloc(sizeof *s);assert(s);speaker_init(s,rate,.15f);assert(!speaker_enqueue(s,&score));
   for(size_t i=0;i<n;) {size_t chunk=n-i<137?n-i:137;speaker_float(s,actual+i,chunk);i+=chunk;}
   for(size_t i=0;i<n;i++) {double error=fabs(reference[i]-actual[i]);if(error>max_error)max_error=error;assert(error<.000001);}
   float silence[32];speaker_float(s,silence,32);for(unsigned i=0;i<32;i++)assert(silence[i]==0);
   int16_t stereo[64];speaker_init(s,rate,.15f);assert(!speaker_enqueue(s,&score));speaker_s16(s,stereo,32,2);
   for(unsigned i=0;i<32;i++){assert(stereo[i*2]==stereo[i*2+1]);assert(fabs(stereo[i*2]/32767.f-reference[i])<.00004);}
   free(reference);free(actual);free(s);
  }
 }
 printf("OK: streamed PIT matches all 7 reference effects at 44.1/48 kHz, arbitrary callback boundaries, stereo and silence; maximum error %.9g (%g s).\n",max_error,(double)(clock()-start)/CLOCKS_PER_SEC);
 return 0;
}
