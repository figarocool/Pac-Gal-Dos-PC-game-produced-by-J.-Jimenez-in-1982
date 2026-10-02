#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
 Game g;game_init(&g,1982);
 FILE *f=fopen("docs/original-screen.bin","rb");if(!f)return 1;
 unsigned char screen[4148];size_t n=fread(screen,1,sizeof screen,f);fclose(f);
 if(n<4000)return 1;
 for(int y=0;y<24;y++)for(int x=0;x<80;x++)
  if(g.ch[y][x]!=screen[y*160+x*2]||g.attr[y][x]!=screen[y*160+x*2+1]) {
   fprintf(stderr,"Maze mismatch at %d,%d\n",y,x);return 1;
  }
 const int dy[]={0,-1,0,1},dx[]={-1,0,1,0};
 for(int d=0;d<4;d++){game_direction(&g,dy[d],dx[d]);for(int j=0;j<16;j++){game_player_step(&g);game_ghost_step(&g); }}
 f=fopen("docs/original-moves.bin","rb");if(!f)return 1;
 n=fread(screen,1,sizeof screen,f);fclose(f);if(n!=4148)return 1;
 int errors=0;
 for(int y=0;y<25;y++)for(int x=0;x<80;x++)
  if(g.ch[y][x]!=screen[y*160+x*2]||g.attr[y][x]!=screen[y*160+x*2+1]) {
   fprintf(stderr,"Movement mismatch at %d,%d: native %d/%d, DOS %d/%d\n",y,x,g.ch[y][x],g.attr[y][x],screen[y*160+x*2],screen[y*160+x*2+1]);errors++;
  }
 int lives=screen[4000]|screen[4001]<<8,dots=screen[4002]|screen[4003]<<8;
 if(lives!=g.lives||dots!=g.dots){fprintf(stderr,"Counters: C %d/%d DOS %d/%d\n",g.lives,g.dots,lives,dots);errors++;}
 for(int i=0;i<4;i++) {
  const Actor *a=&g.ghost[i];
  int values[]={i+3,a->color,a->row,a->col,a->under,a->timer,a->dy,a->dx,0,0};
  for(int j=0;j<8;j++) {
   int k=4028+2*(j*6+i+2);
   int v=(int)(int16_t)(screen[k]|screen[k+1]<<8);
   if(v!=values[j]){fprintf(stderr,"Ghost %d field %d: C %d DOS %d\n",i,j,values[j],v);errors++;}
  }
 }
 unsigned rng=screen[4024]|screen[4025]<<8|screen[4026]<<16;
 if(rng!=g.rng){fprintf(stderr,"PRNG mismatch: C %x DOS %x\n",g.rng,rng);errors++;}
 if(errors)return 1;
 puts("OK: maze/colours and 64 player/ghost steps match real DOS framebuffer, counters, ghost state and PRNG.");return 0;
}
