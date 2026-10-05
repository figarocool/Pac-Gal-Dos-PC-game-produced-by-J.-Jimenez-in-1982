/* Native C implementation of the historical game's behavior.
   Analysis references and instruction offsets: docs/reverse-engineering.md. */
#include "game.h"
#include "audio.h"
#include "maze.h"
#include "i18n.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
static int sign(int n) {return (n>0)-(n<0);}
static float random_value(Game *g) {
 /* The original BASIC runtime at 0x09C1 uses a 24-bit LCG. */
 g->rng=(g->rng*214013u+1744579u)&0xffffffu;
 return (float)g->rng/16777216.f;
}
static unsigned random_bit(Game *g) {return random_value(g)>.5f;}
static void put(Game *g,int row,int col,int ch,int color) {
 if(row<1||row>25||col<0||col>=80)return;
 g->ch[row-1][col]=(unsigned char)ch;
 g->attr[row-1][col]=(unsigned char)((color&15)|((color&16)<<3));
}
static int tile(const Game *g,int row,int col) {
 if(row<1||row>24||col<0||col>39)return 219;
 return g->ch[row-1][col*2];
}
static int attr(const Game *g,int row,int col) {
 if(row<1||row>24||col<0||col>39)return 7;
 return g->attr[row-1][col*2];
}
static int floor_tile(int c) {return c==32||c==249;}
static int remix_portal_target(const Game *g,int *row,int *col) {
 if(!g->remix)return 0;
 if(*row==g->tunnel_left_row&&*col==0) {
  *row=g->tunnel_right_row;*col=38;return 1;
 }
 if(*row==g->tunnel_right_row&&*col==39) {
  *row=g->tunnel_left_row;*col=1;return 1;
 }
 return 0;
}
static void event(Game *g,int kind) {
 g->sound=kind;
 if(g->event_count<64)g->events[g->event_count++]=kind;
}
void game_replay_prompt(Game *g) {
 #ifdef VITA
 const char *s=i18n_text(I18N_REPLAY_VITA);
 #else
 const char *s=i18n_text(I18N_REPLAY_PC);
 #endif
 for(int c=9;*s;c++)put(g,25,c,*s++,26);
}
void game_status(Game *g) {
 if(g->status_dots!=g->dots) {
  char s[32];snprintf(s,sizeof s," %d ",g->dots);
  const char *label=i18n_text(I18N_DOTS);for(int c=0;label[c];c++)put(g,25,c,label[c],7);
  for(int c=0;s[c];c++)put(g,25,(int)strlen(label)+c,s[c],7);
  g->status_dots=g->dots;
 }
 if(g->status_lives!=g->lives) {
  for(int c=0;c<g->lives;c++)put(g,25,14+c,1,7);
  if(g->status_lives) {
   int end=g->sound==SOUND_DEATH?28:18+g->lives;
   for(int c=14+g->lives;c<end;c++)put(g,25,c,32,7);
  }
  g->status_lives=g->lives;
 }
 if(g->ended==1) {
  const char *s=i18n_text(I18N_VICTORY);for(int c=9;*s;c++)put(g,25,c,*s++,26);
 } else if(g->ended==2)game_replay_prompt(g);
}
static void initialize(Game *g,uint32_t seed,unsigned low_byte) {
 memset(g,0,sizeof *g);g->rng=((seed&65535u)<<8)|(low_byte&255u);g->lives=3;g->dots=468;
 maze_initialize(g);
 memset(g->attr[24],7,80);
 const unsigned char title[]="    P A C - G A L         Al J. Jim\202nez, May 1982";
 for(unsigned c=0;c<sizeof(title)-1;c++)put(g,25,27+(int)c,title[c],c<18?15:7);
 const char *label=i18n_text(I18N_DOTS);for(int c=0;label[c];c++)put(g,25,c,label[c],7);
 g->status_dots=-1;
 g->player=(Actor){19,19,0,0,32,7,0};
 for(int i=0;i<4;i++) {
  int s=random_bit(g)?1:-1, axis=(int)random_bit(g);
  g->ghost[i]=(Actor){14,i+18,axis*s,(1-axis)*s,32,7,0};
 }
 game_status(g);
}
void game_init(Game *g,uint32_t seed) {initialize(g,seed,5);}
/* Remix maps use a randomized depth-first maze inside a rectangular border.
   The spanning tree connects every node; opening extra walls widens routes. */
void game_init_remix(Game *g,uint32_t seed,unsigned level) {
 memset(g,0,sizeof *g);g->rng=((seed&65535u)<<8)|5u;g->remix=1;g->level=(int)level;
 g->lives=3;g->status_dots=-1;g->status_lives=0;
 memset(g->ch,219,sizeof g->ch);memset(g->attr,7,sizeof g->attr);
 /* Remix walls fill the playfield, but the footer starts as empty black
    cells like the original game's status row. */
 memset(g->ch[24],32,sizeof g->ch[24]);
 int seen[11][19]={{0}},sr[209],sc[209],top=1;
 sr[0]=3;sc[0]=1;seen[0][0]=1;put(g,3,2,32,7);
 while(top) {
  int r=sr[top-1],c=sc[top-1],opts[4],n=0;
  const int dr[4]={-2,2,0,0},dc[4]={0,0,-2,2};
  for(int d=0;d<4;d++){int nr=r+dr[d],nc=c+dc[d];if(nr>=3&&nr<=23&&nc>=1&&nc<=37&&!seen[(nr-3)/2][(nc-1)/2])opts[n++]=d;}
  if(!n){top--;continue;}
  int d=opts[(int)(random_value(g)*n)],nr=r+dr[d],nc=c+dc[d];
  seen[(nr-3)/2][(nc-1)/2]=1;put(g,r+dr[d]/2,(c+dc[d]/2)*2,32,7);put(g,nr,nc*2,32,7);
  sr[top]=nr;sc[top++]=nc;
 }
 /* Widen selected passages while preserving the connected backbone. */
 for(int r=2;r<24;r++)for(int c=2;c<39;c++)if(g->ch[r-1][c*2]==219&&random_value(g)<.42f) {
  int horiz=(g->ch[r-1][(c-1)*2]==32&&g->ch[r-1][(c+1)*2]==32);
  int vert=(r>1&&r<24&&g->ch[r-2][c*2]==32&&g->ch[r][c*2]==32);
  if(horiz||vert)put(g,r,c*2,32,7);
 }
 /* Row 2 was left as a solid block by the initial wall fill, making the
    upper perimeter two cells thick. Turn it into an open lane just inside
    the top rail after random carving, so it does not affect the RNG stream. */
 for(int c=1;c<78;c++)put(g,2,c,32,7);
 /* Column 38 lies outside the randomized DFS grid and otherwise stays a
    second solid wall just inside the right perimeter. Open it after random
    carving so the outside frame is only one cell thick. */
 for(int r=3;r<=23;r++)put(g,r,76,32,7);
 /* A small open ghost pen; all its cells meet the connected maze. */
 for(int r=12;r<=15;r++)for(int c=17;c<=22;c++)put(g,r,c*2,32,7);
 /* Clear a short approach outside the pen door so random walls cannot seal
    the route immediately after enemies leave their starting room. */
 for(int r=9;r<=11;r++)for(int c=19;c<=20;c++)put(g,r,c*2,32,7);
 /* Widen the center row of that approach to keep the cells above both upper
    pen corners connected when the random maze leaves them as walls. */
 for(int c=17;c<=22;c++)put(g,10,c*2,32,7);
 /* Keep one-cell bypass lanes along both outer sides of the pen: random
    walls can otherwise isolate pellets beside its frame. */
 for(int r=9;r<=19;r++){put(g,r,30,32,7);put(g,r,48,32,7);}
 /* The short route beneath the pen joins those two bypasses. */
 for(int c=17;c<=22;c++)put(g,18,c*2,32,7);
 /* Reserve a two-cell doorway on top and keep random corridors out of the
    pen's frame. The surrounding maze remains connected around this room. */
 put(g,12,16*2,201,7);put(g,12,23*2,187,7);
 for(int c=17;c<=18;c++)put(g,12,c*2,205,7);
 for(int c=21;c<=22;c++)put(g,12,c*2,205,7);
 for(int r=13;r<=15;r++){put(g,r,16*2,186,7);put(g,r,23*2,186,7);}
 put(g,16,16*2,200,7);put(g,16,23*2,188,7);
 for(int c=17;c<=22;c++)put(g,16,c*2,205,7);
 /* Paired side exits vary on every map and always land just inside the frame. */
 g->tunnel_left_row=3+2*(int)(random_value(g)*11.f);
 g->tunnel_right_row=3+2*(int)(random_value(g)*11.f);
 if(g->tunnel_right_row==g->tunnel_left_row)
  g->tunnel_right_row=g->tunnel_right_row==23?3:g->tunnel_right_row+2;
 put(g,g->tunnel_left_row,0,196,7);put(g,g->tunnel_right_row,78,196,7);
 put(g,g->tunnel_left_row,2,32,7);put(g,g->tunnel_right_row,76,32,7);
 put(g,g->tunnel_right_row,74,32,7);
 /* Put the player on a known open maze node and energizers in its corners. */
 put(g,19,38,32,7);g->player=(Actor){19,19,0,0,32,7,0};
 int powers[4][2]={{3,3},{3,35},{23,3},{23,35}};
 for(int r=2;r<24;r++)for(int c=1;c<39;c++)if(g->ch[r-1][c*2]==32) {
  int power=0;for(int i=0;i<4;i++)if(powers[i][0]==r&&powers[i][1]==c)power=1;
  if(!(r>=12&&r<=15&&c>=17&&c<=22)){put(g,r,c*2,249,power?26:7);g->dots++;}
 }
 g->player.under=tile(g,19,19);put(g,19,38,1,7);
 for(int i=0;i<4;i++) {
  g->ghost[i]=(Actor){14,i+18,0,i%2?1:-1,32,7,0};
  put(g,14,(i+18)*2,i+3,7);
 }
 /* Each logical maze cell occupies two text columns. Fill the second column
    from its neighbours: floor joins smoothly, while adjacent wall cells
    keep the frame and internal walls visually solid. */
 for(int r=1;r<=24;r++)for(int c=0;c<39;c++) {
  int left=g->ch[r-1][c*2],right=g->ch[r-1][(c+1)*2];
  int left_wall=left==219,right_wall=right==219;
  put(g,r,c*2+1,left_wall&&right_wall?219:32,7);
 }
 /* Match the original single-character frame on both sides. The half-cell
    neighbor fill is random, so reset it explicitly; keep the corner rows
    solid horizontally and leave the unused final column blank. */
 for(int r=1;r<=24;r++) {
  if(r==1||r==24){put(g,r,79,32,7);continue;}
  if(r!=g->tunnel_left_row)put(g,r,0,219,7);
  put(g,r,1,32,7);
  if(r!=g->tunnel_right_row)put(g,r,78,219,7);
  put(g,r,79,32,7);
 }
 /* Use the same thin CP437 horizontal rails as the original maze instead of
    full-block cells, which make the Remix perimeter look double-thick. */
 for(int c=0;c<79;c++){put(g,1,c,220,7);put(g,24,c,223,7);}
 put(g,1,79,32,7);put(g,24,79,32,7);
 /* The outer right edge occupies the final two columns; the right portal is
    the sole opening in that side of the rectangular enclosure. */
 put(g,g->tunnel_right_row,78,32,7);put(g,g->tunnel_right_row,79,32,7);
 put(g,g->tunnel_left_row,0,196,7);put(g,g->tunnel_left_row,1,32,7);
 put(g,g->tunnel_right_row,78,196,7);put(g,g->tunnel_right_row,79,32,7);
 /* Complete the reserved pen outline in the unused half-columns; its only
    entrance is the two-cell opening at the top. */
 put(g,12,33,205,7);put(g,12,35,205,7);put(g,12,37,205,7);
 put(g,12,43,205,7);put(g,12,45,205,7);
 /* One glyph on each side: drawing the unused half-cell as a second vertical
    line makes the border look like an impassable divider inside the room. */
 put(g,16,33,205,7);
 for(int c=35;c<=45;c+=2)put(g,16,c,205,7);
 /* The pen interior is a clean room even when nearby random passages align
    with its half-character separators. */
 for(int r=13;r<=15;r++)for(int c=35;c<=45;c+=2)put(g,r,c,32,7);
 const char *label=i18n_text(I18N_DOTS);for(int c=0;label[c];c++)put(g,25,c,label[c],7);
 char title[50];snprintf(title,sizeof title,i18n_text(I18N_LEVEL_FMT),level);
 for(unsigned c=0;c<strlen(title);c++)put(g,25,27+(int)c,title[c],15);
 game_status(g);
}
void game_restart(Game *g,uint32_t seed) {
 /* Original CLS excludes row 25. Preserve its leftover characters, even
    the original replay-message remnants, then overwrite only PRINT ranges. */
 unsigned char chars[80],attrs[80];memcpy(chars,g->ch[24],80);memcpy(attrs,g->attr[24],80);
 unsigned low_byte=g->rng&255u;initialize(g,seed,low_byte);
 for(int c=0;c<80;c++)if(!((c<=8)||(c>=14&&c<=16)||(c>=27&&c<=75))) {
  g->ch[24][c]=chars[c];g->attr[24][c]=attrs[c];
 }
}
void game_direction(Game *g,int dy,int dx) {
 /* BASIC stores desired direction independently from current velocity. */
 g->requested_dy=dy;g->requested_dx=dx;
}
static void snapshot(Game *g,int effect) {
 memcpy(g->effect_ch,g->ch,sizeof g->ch);memcpy(g->effect_attr,g->attr,sizeof g->attr);g->effect=effect;
}
static void die(Game *g,int row,int col) {
 put(g,g->player.row,g->player.col*2,g->player.under,7);
 put(g,row,col*2,168,26);snapshot(g,SOUND_DEATH);event(g,SOUND_DEATH);
 if(g->dots>300&&g->aggression<.1f)g->aggression*=2;
 if(g->lives<=1){g->ended=2;game_status(g);return;}
 --g->lives;
 for(int i=0;i<4;i++) {
  Actor *a=&g->ghost[i];put(g,a->row,a->col*2,a->under,7);
  a->under=32;a->color=7;a->row=14;a->col=i+19;a->timer=0;
 }
 g->player.row=19;g->player.col=19;g->player.under=32;
 g->player.dy=g->player.dx=g->requested_dy=g->requested_dx=0;
 game_status(g);
}
static void eat(Game *g,int row,int col) {
 put(g,g->player.row,g->player.col*2,g->player.under,7);
 put(g,row,col*2,2,26);snapshot(g,SOUND_EAT);
 event(g,SOUND_EAT);g->aggression*=.5f;
 if(g->lives<3)g->lives++;
 for(int i=0;i<4;i++) {
  Actor *a=&g->ghost[i];a->timer=(int16_t)(a->timer-1);
  if(a->row==row&&a->col==col) {
   a->color=7;a->row=14;a->col=i+18;
   if(a->under==249)g->dots--;
   a->under=32;
  }
 }
}
void game_player_step(Game *g) {
 if(g->ended)return;
 Actor *p=&g->player;
 int row=p->row+g->requested_dy,col=p->col+g->requested_dx;
 int ch=tile(g,row,col), desired=0;
 if(remix_portal_target(g,&row,&col)){ch=tile(g,row,col);desired=floor_tile(ch);}
 if(!desired&&(floor_tile(ch)||(ch>2&&ch<7)))desired=1;
 else if(row==12&&ch==196) {
  col=39-p->col;ch=tile(g,row,col);desired=floor_tile(ch);
 }
 if(!desired) {
  row=p->row+p->dy;col=p->col+p->dx;ch=tile(g,row,col);
  if(remix_portal_target(g,&row,&col))ch=tile(g,row,col);
  if(!floor_tile(ch)&&!(ch>2&&ch<7)&&row==12&&ch==196) {
   col=39-p->col;ch=tile(g,row,col);
  }
  if(!floor_tile(ch)&&!(ch>2&&ch<7)) {
   p->dy=p->dx=0;event(g,SOUND_WALL);
   /* Original RETURN falls through into the desired-direction path
      after SOUND; this preserves the resulting stationary move. */
   return;
  }
 }
 if(ch>2&&ch<7) {
  if(attr(g,row,col)<=7){die(g,row,col);return;}
  eat(g,row,col);desired=0;
 } else if(attr(g,row,col)>7) {
  int t=(int)nearbyintf(((float)g->dots/5.f+20.f)/(float)(g->lives*g->lives));
  for(int i=0;i<4;i++){g->ghost[i].color=26;g->ghost[i].timer=t;}
  event(g,SOUND_POWER);
 }
 if(desired){p->dy=g->requested_dy;p->dx=g->requested_dx;}
 put(g,p->row,p->col*2,p->under,7);
 put(g,row,col*2,1,7);p->row=row;p->col=col;p->under=32;
 if(ch==249&&g->dots>0){g->dots--;event(g,SOUND_DOT);}
 else {int previous=g->sound;event(g,SOUND_FLOOR);if(previous==SOUND_EAT)g->sound=previous;}
 if(g->dots<1)g->ended=1;
 if(g->dots<50)g->aggression*=.5f;
 game_status(g);
}
static void ghost_collision(Game *g,Actor *a) {
 int row=a->row,col=a->col;
 if(a->color<=7){die(g,row,col);return;}
 eat(g,row,col);
 put(g,g->player.row,g->player.col*2,g->player.under,7);
 put(g,row,col*2,1,7);
 g->player.row=row;g->player.col=col;g->player.under=32;
 event(g,SOUND_FLOOR);g->sound=SOUND_EAT;
 if(g->dots<1)g->ended=1;
 if(g->dots<50)g->aggression*=.5f;
 game_status(g);
}
/* Candidate order follows 0x28A8..0x2BF6: pursuit (when enabled),
   straight, randomized perpendicular, opposite perpendicular, reverse. */
void game_ghost_step(Game *g) {
 if(g->ended)return;
 for(int i=0;i<4;i++) {
  Actor *a=&g->ghost[i];a->timer=(int16_t)(a->timer-1);
  int r=a->row,c=a->col, chosen=0, ch=a->under;
  int nr=r,nc=c,move_dy=a->dy,move_dx=a->dx;
  int centre=c>=18&&c<=23;
  int turn=centre&&r>12&&r<16;
  int chase=0;
  if(!(centre&&(r==11||r==12))&&!turn)
   chase=random_value(g)>g->aggression;
  int dy[6],dx[6],n=0;
  if(chase) {
   if(a->dy==0&&r!=g->player.row){dy[n]=sign(g->player.row-r);dx[n++]=0;}
   if(a->dx==0&&c!=g->player.col){dy[n]=0;dx[n++]=sign(g->player.col-c);}
  }
  if(!turn){dy[n]=a->dy;dx[n++]=a->dx;}
  /* Consume the turn random number only after pursuit/straight fails. */
  for(int j=0;j<n;j++) {
   int tr=r+dy[j],tc=c+dx[j],t=tile(g,tr,tc);
   if(remix_portal_target(g,&tr,&tc))t=tile(g,tr,tc);
   if(t==1){ghost_collision(g,a);return;}
   if(floor_tile(t)){nr=tr;nc=tc;ch=t;move_dy=dy[j];move_dx=dx[j];chosen=1;break;}
  }
  if(!chosen) {
   int s=random_bit(g)?1:-1;
   int turns_y[]={a->dx*s,-a->dx*s,-a->dy};
   int turns_x[]={a->dy*s,-a->dy*s,-a->dx};
   for(int j=0;j<3;j++) {
    int tr=r+turns_y[j],tc=c+turns_x[j],t=tile(g,tr,tc);
    if(remix_portal_target(g,&tr,&tc))t=tile(g,tr,tc);
    if(t==1){ghost_collision(g,a);return;}
    if(floor_tile(t)){nr=tr;nc=tc;ch=t;move_dy=turns_y[j];move_dx=turns_x[j];chosen=1;break;}
   }
  }
  if(chosen){a->dy=move_dy;a->dx=move_dx;}
  put(g,r,c*2,a->under,7);
  if(a->timer<=0)a->color=7;
  put(g,nr,nc*2,i+3,a->color);
  a->row=nr;a->col=nc;a->under=ch;
 }
 g->ticks++;
}
