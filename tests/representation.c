/* Emit observable output for comparison between two source revisions.
   Includes every construction step and inherited music parser state. */
#include "presentation.h"
#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
static void music_state(const BasicMusic *m) {
 unsigned values[]={m->tempo,m->length,m->octave,m->articulation,(unsigned)m->background};
 fwrite(values,sizeof values,1,stdout);
}
int main(void) {
 Game g;screen_blank(&g);unsigned drawn=0,count=screen_build_count();
 fwrite(&count,sizeof count,1,stdout);
 for(unsigned n=0;n<=count;n++) {
  screen_build_to(&g,&drawn,n);
  fwrite(g.ch,sizeof g.ch,1,stdout);fwrite(g.attr,sizeof g.attr,1,stdout);
 }
 /* Also exercise batches, the final clamp, and preservation of existing cells. */
 screen_blank(&g);g.ch[0][79]=42;drawn=0;
 for(unsigned n=17;n<count+17;n+=17)screen_build_to(&g,&drawn,n);
 screen_build_to(&g,&drawn,count+100);
 fwrite(g.ch,sizeof g.ch,1,stdout);fwrite(g.attr,sizeof g.attr,1,stdout);
 /* Isolated effects also cover inherited octave/tempo and queue exhaustion. */
 for(unsigned articulation=1;articulation<=3;articulation++)
  for(unsigned octave=0;octave<=6;octave++)
   for(int event=SOUND_DOT;event<=SOUND_INTRO;event++)
    for(unsigned capacity=0;capacity<3;capacity++) {
     BasicMusic m={173,16,octave,articulation,0};BasicScore score={0};
     score.count=capacity==0?0:capacity==1?BASIC_AUDIO_MAX-1:BASIC_AUDIO_MAX;
     int result=basic_effect(&m,&score,event);
     fwrite(&result,sizeof result,1,stdout);music_state(&m);
     fwrite(&score.count,sizeof score.count,1,stdout);
     fwrite(score.tone,sizeof *score.tone,score.count,stdout);
    }
 for(unsigned articulation=1;articulation<=3;articulation++) {
  for(unsigned octave=0;octave<=6;octave++) {
   BasicMusic m={173,16,octave,articulation,0};
   int events[]={SOUND_INTRO,SOUND_DOT,SOUND_POWER,SOUND_WALL,SOUND_FLOOR,SOUND_DEATH,SOUND_EAT};
   for(unsigned i=0;i<sizeof events/sizeof *events;i++) {
    BasicScore score={0};if(basic_effect(&m,&score,events[i]))return 1;
    music_state(&m);fwrite(&score.count,sizeof score.count,1,stdout);
    fwrite(score.tone,sizeof *score.tone,score.count,stdout);
    unsigned rates[]={22050,44100,48000};
    for(unsigned r=0;r<3;r++) {
     size_t n=basic_samples(&score,rates[r]);float *samples=malloc(n*sizeof *samples);
     if(!samples&&n)return 1;
     basic_render(&score,samples,rates[r],.15f);
     fwrite(samples,sizeof *samples,n,stdout);free(samples);
    }
   }
  }
 }
 return ferror(stdout)?1:0;
}
