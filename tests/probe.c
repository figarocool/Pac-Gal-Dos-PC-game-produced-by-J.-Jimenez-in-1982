#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"r");if(!f)return 2;
 unsigned seed;int dy,dx,count,ghosts,effect=0;fscanf(f,"%u %d",&seed,&ghosts);
 Game g;game_init(&g,seed);
 char cmd[16];int x,y,z,w;
 while(fscanf(f,"%15s",cmd)==1) {
  if(!strcmp(cmd,"move")){fscanf(f,"%d %d %d",&dy,&dx,&count);game_direction(&g,dy,dx);while(count--){game_player_step(&g);if(ghosts)game_ghost_step(&g);}}
  else if(!strcmp(cmd,"replay")){game_replay_prompt(&g);game_restart(&g,seed);}
  else if(!strcmp(cmd,"effect")){effect=1;}
  else if(!strcmp(cmd,"ghoststep")){game_ghost_step(&g);}
  else if(!strcmp(cmd,"lives")){fscanf(f,"%d",&g.lives);g.status_lives=g.lives;}
  else if(!strcmp(cmd,"dots")){fscanf(f,"%d",&g.dots);}
  else if(!strcmp(cmd,"actor")){fscanf(f,"%d %d %d",&x,&y,&z);Actor *a=x?&g.ghost[x-1]:&g.player;int *v[]={&a->row,&a->col,&a->dy,&a->dx,&a->under,&a->color,&a->timer};*v[y]=z;}
  else if(!strcmp(cmd,"cell")){fscanf(f,"%d %d %d %d",&x,&y,&z,&w);g.ch[x][y]=z;g.attr[x][y]=w;}
  else return 2;
 }
 fclose(f);f=fopen(argv[2],"wb");if(!f)return 2;
 for(int r=0;r<25;r++)for(int c=0;c<80;c++){fputc(effect?g.effect_ch[r][c]:g.ch[r][c],f);fputc(effect?g.effect_attr[r][c]:g.attr[r][c],f);}
 int state[2]={g.lives,g.dots};for(int i=0;i<2;i++){fputc(state[i]&255,f);fputc(state[i]>>8,f);}
 for(int j=0;j<8;j++)for(int i=0;i<5;i++){
  Actor *a=i?&g.ghost[i-1]:&g.player;int v[]={i?i+2:1,a->color,a->row,a->col,a->under,a->timer,a->dy,a->dx};fputc(v[j]&255,f);fputc((v[j]>>8)&255,f);
 }
 fputc(g.rng&255,f);fputc((g.rng>>8)&255,f);fputc((g.rng>>16)&255,f);fclose(f);return 0;
}
