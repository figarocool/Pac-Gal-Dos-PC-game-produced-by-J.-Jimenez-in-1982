/* Historical map built by strokes rather than an embedded screen dump.
   See build.h for provenance and the shared construction description. */
#ifndef PACGAL_MAZE_H
#define PACGAL_MAZE_H
#include "build.h"
#include <string.h>
static inline void maze_initialize(Game *game) {
 /* The status row is initialized separately by game.c. */
 memset(game->ch,MAZE_SPACE,24*80);memset(game->attr,7,24*80);
 maze_construct(game,0,~0u,24);
}
#endif
