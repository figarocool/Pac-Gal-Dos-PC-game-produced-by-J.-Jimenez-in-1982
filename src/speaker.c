/* Incremental PIT synthesis: no allocation or per-sample double/floor calls.
   Integer phase units are 1/sample_rate of a PIT clock; integrating the high
   pulse over a sample preserves the original odd-divisor mode-3 waveform. */
#include "speaker.h"
#include <string.h>
#include <math.h>
void speaker_init(Speaker *s,unsigned rate,float volume) {
 memset(s,0,sizeof *s);s->rate=rate;s->volume=volume;
}
static SpeakerFilter filter_coefficients(unsigned rate,float hz,int high,int order) {
 float k=tanf(3.14159265358979323846f*hz/rate);
 SpeakerFilter f={0};
 if(order==1) {
  float n=1.f/(1.f+k);f.b0=high?n:k*n;
  f.b1=high?-f.b0:f.b0;f.a1=(k-1.f)*n;
 } else {
  /* Third-order Butterworth = one real pole and a Q=1 pole pair. */
  float n=1.f/(1.f+k+k*k);f.b0=high?n:k*k*n;
  f.b1=(high?-2.f:2.f)*f.b0;f.b2=f.b0;
  f.a1=2.f*(k*k-1.f)*n;f.a2=(1.f-k+k*k)*n;
 }
 return f;
}
void speaker_pc_init(Speaker *s,unsigned rate,float volume) {
 speaker_init(s,rate,volume);s->pc_model=1;
 s->irq_until=s->irq_period=65536ull*rate;
 /* DOSBox Staging small-speaker reference: 3rd-order HP 120 Hz,
    3rd-order LP 4300 Hz. This is a response model, not an IBM measurement. */
 s->filter[0]=filter_coefficients(rate,120,1,1);
 s->filter[1]=filter_coefficients(rate,120,1,2);
 s->filter[2]=filter_coefficients(rate,4300,0,1);
 s->filter[3]=filter_coefficients(rate,4300,0,2);
 /* Disabled port-61 output is low. Prime the DC rejection at that level. */
 s->filter[0].z1=s->filter[0].b0*volume;
}
void speaker_video_init(Speaker *s,unsigned rate,float volume) {
 speaker_pc_init(s,rate,volume);
 s->pc_model=2;s->dc_input=-volume;
 /* The supplied DOSBox recording has a much slower DC-decay tail than
    the representative small-speaker HP120 model (about 2.6--2.8 Hz).
    Use a 3 Hz DC blocker for this reference, retaining the LP4300 response.
    This is a recording-reference profile, not a measured IBM enclosure. */
 s->filter[0]=filter_coefficients(rate,3,1,1);
 s->filter[0].z1=s->filter[0].b0*volume;
 s->filter[1]=(SpeakerFilter){.b0=1};
}
int speaker_enqueue(Speaker *s,const BasicScore *score) {
 if(score->count>SPEAKER_QUEUE-(s->head-s->tail))return -1;
 uint64_t ticks=0;unsigned first=0;
 for(unsigned i=0;i<score->count;i++) {
  ticks+=score->tone[i].ticks;
  unsigned end=(unsigned)(ticks*BASIC_SOUND_TICK*s->rate/BASIC_PIT_HZ);
  s->queue[s->head++%SPEAKER_QUEUE]=(SpeakerPacket){score->tone[i].hz,end-first,score->tone[i].ticks};first=end;
 }
 return 0;
}
static float raw_sample(Speaker *s) {
 while(!s->remaining) {
  if(s->tail==s->head)return 0;
  SpeakerPacket p=s->queue[s->tail++%SPEAKER_QUEUE];s->remaining=p.samples;s->phase=0;
  unsigned divisor=p.hz?BASIC_PIT_HZ/p.hz:0;
  s->period=(uint64_t)divisor*s->rate;s->high=(uint64_t)((divisor+1)/2)*s->rate;
 }
 --s->remaining;if(!s->period)return 0;
 uint64_t end=s->phase+BASIC_PIT_HZ;
 uint64_t area=0,a=s->phase;
 while(end>=s->period) {
  if(a<s->high)area+=s->high-a;
  end-=s->period;a=0;
 }
 uint64_t b=end<s->high?end:s->high;
 if(b>a)area+=b-a;
 s->phase=end;
 return s->volume*((float)area*(2.f/BASIC_PIT_HZ)-1.f);
}
static void irq_advance(Speaker *s,int sounding) {
 uint64_t left=BASIC_PIT_HZ;
 while(left>=s->irq_until) {
  left-=s->irq_until;s->irq_until=s->irq_period;
  if(sounding&&s->remaining)s->remaining--;
 }
 s->irq_until-=left;
}
static float pc_pin_sample(Speaker *s) {
 while(!s->remaining) {
  if(s->tail==s->head) {
   s->gate=0;s->irq_period=65536ull*s->rate;
   irq_advance(s,0);return -s->volume;
  }
  SpeakerPacket p=s->queue[s->tail++%SPEAKER_QUEUE];s->remaining=p.ticks;
  /* BASIC queues divisor 2 for a rest: ultrasonic, effectively silent.
     Keep its timer state rather than switching off the speaker gate. */
  unsigned divisor=p.hz?BASIC_PIT_HZ/p.hz:2;
  uint64_t period=(uint64_t)divisor*s->rate;
  uint64_t high=(uint64_t)((divisor+1)/2)*s->rate;
  if(!s->gate) {
   /* Original writes PIT0 count 2048 without a new control word.
      Keep the already scheduled BIOS IRQ; only later IRQs run faster.
      This makes an isolated short SOUND depend on the previous IRQ phase. */
   s->irq_period=(uint64_t)BASIC_SOUND_TICK*s->rate;
   s->gate=1;s->phase=0;s->period=period;s->high=high;s->pending_period=0;
  } else {s->pending_period=period;s->pending_high=high;}
 }
 uint64_t left=BASIC_PIT_HZ,area=0;
 while(left) {
  int high=s->phase<s->high;
  uint64_t edge=high?s->high:s->period;
  uint64_t span=edge-s->phase;if(span>left)span=left;
  if(high)area+=span;
  s->phase+=span;left-=span;
  if(s->phase==edge) {
   /* Mode 3 loads a replacement count at a half-period boundary;
      a new note never resets an already enabled timer mid-pulse. */
   if(s->pending_period) {
    s->period=s->pending_period;s->high=s->pending_high;s->pending_period=0;
   }
   s->phase=high?s->high:0;
  }
 }
 irq_advance(s,1);
 return s->volume*((float)area*(2.f/BASIC_PIT_HZ)-1.f);
}
static float next_sample(Speaker *s) {
 if(!s->pc_model)return raw_sample(s);
 float value=pc_pin_sample(s);
 unsigned first=0;
 if(s->pc_model==2) {
  /* Difference form avoids a float DC residual at such a low cutoff. */
  float out=-s->filter[0].a1*s->dc_output+s->filter[0].b0*(value-s->dc_input);
  s->dc_input=value;s->dc_output=out;value=out;first=1;
  if(fabsf(s->dc_output)<1e-20f)s->dc_output=0;
 }
 for(unsigned i=first;i<4;i++) {
  SpeakerFilter *f=&s->filter[i];float out=f->b0*value+f->z1;
  f->z1=f->b1*value-f->a1*out+f->z2;f->z2=f->b2*value-f->a2*out;
  /* Avoid denormal arithmetic when the impulse response has decayed. */
  if(fabsf(f->z1)<1e-20f)f->z1=0;
  if(fabsf(f->z2)<1e-20f)f->z2=0;
  value=out;
 }
 return value;
}
void speaker_float(Speaker *s,float *samples,size_t count) {
 for(size_t i=0;i<count;i++)samples[i]=next_sample(s);
}
void speaker_s16(Speaker *s,int16_t *samples,size_t frames,unsigned channels) {
 for(size_t i=0;i<frames;i++) {
  float value=next_sample(s)*32767.f;
  if(value>32767)value=32767;
  if(value< -32768)value=-32768;
  int16_t pcm=(int16_t)value;
  for(unsigned c=0;c<channels;c++)samples[i*channels+c]=pcm;
 }
}
