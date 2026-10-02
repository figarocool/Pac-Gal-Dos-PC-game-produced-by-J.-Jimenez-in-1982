#include "timing.h"
/* Measured from the supplied gameplay video: its movement sound events recur
   about every 350 ms at the displayed speed 1000. The mapping is host-stable. */
unsigned game_period_ms(int speed) {
 if(speed<0)speed=0;
 if(speed>30000)speed=30000;
 return 300u+(unsigned)speed/20u;
}
static void advance(GameTiming *t,uint64_t now) {
 if(!t->paused&&now>=t->last_ms)t->elapsed_ms+=now-t->last_ms;
 t->last_ms=now;
}
void game_timing_init(GameTiming *t,uint64_t now,int speed) {
 t->last_ms=now;t->elapsed_ms=0;t->paused=0;
 t->period_ms=game_period_ms(speed);t->next_ms=now+t->period_ms;
}
void game_timing_speed(GameTiming *t,uint64_t now,int speed) {
 advance(t,now);t->period_ms=game_period_ms(speed);
 t->next_ms=now+t->period_ms;
}
void game_timing_pause(GameTiming *t,uint64_t now,int paused) {
 advance(t,now);t->paused=paused;
 /* Never replay turns accumulated while the game is paused. */
 t->next_ms=now+t->period_ms;
}
int game_timing_update(GameTiming *t,uint64_t now) {
 advance(t,now);
 if(t->paused||now<t->next_ms)return 0;
 t->next_ms+=t->period_ms;
 /* A long render/window stall must not cause a burst of moves. */
 if(t->next_ms<=now)t->next_ms=now+t->period_ms;
 return 1;
}
