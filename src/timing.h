#ifndef PACGAL_TIMING_H
#define PACGAL_TIMING_H
#include <stdint.h>
typedef struct {
 uint64_t last_ms,next_ms,elapsed_ms;
 unsigned period_ms;
 int paused;
} GameTiming;
unsigned game_period_ms(int speed);
void game_timing_init(GameTiming *t,uint64_t now,int speed);
void game_timing_speed(GameTiming *t,uint64_t now,int speed);
void game_timing_pause(GameTiming *t,uint64_t now,int paused);
int game_timing_update(GameTiming *t,uint64_t now);
#endif
