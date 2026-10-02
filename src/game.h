#ifndef PACGAL_GAME_H
#define PACGAL_GAME_H
#include <stdint.h>
/* Actor coordinates are logical maze cells. `under` stores the screen
   character hidden by the actor so game.c can restore it when the actor moves. */
typedef struct { int row,col,dy,dx,under,color,timer; } Actor;
/* Mutable screen plus simulation state; fields are explained in
   docs/guida-al-codice.md. */
typedef struct {
 unsigned char ch[25][80],attr[25][80];
 Actor player,ghost[4];
 /* ended: 0=playing, 1=won, 2=out of lives. Direction input is buffered. */
 int lives,dots,ended,requested_dy,requested_dx; float aggression; uint32_t rng;
 int remix,level,tunnel_left_row,tunnel_right_row;
 unsigned ticks; int sound;
 unsigned char effect_ch[25][80],effect_attr[25][80];
 int effect;
 unsigned event_count; int events[64]; int status_lives,status_dots;
} Game;
void game_init(Game *g,uint32_t seed);
void game_init_remix(Game *g,uint32_t seed,unsigned level);
void game_restart(Game *g,uint32_t seed);
void game_direction(Game *g,int dy,int dx);
void game_player_step(Game *g);
void game_ghost_step(Game *g);
void game_status(Game *g);
void game_replay_prompt(Game *g);
#endif
