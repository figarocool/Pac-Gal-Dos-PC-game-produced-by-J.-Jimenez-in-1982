#ifndef PACGAL_PRESENTATION_H
#define PACGAL_PRESENTATION_H
#include "game.h"
void screen_blank(Game *g);
void screen_text(Game *g,int row,int col,const char *s,int attr);
void screen_speed(Game *g,const char *speed);
void screen_mode(Game *g,int selected);
void screen_intro_frame(Game *g,unsigned frame);
unsigned screen_build_count(void);
void screen_build_to(Game *g,unsigned *drawn,unsigned target);
unsigned screen_build_remix_count(void);
void screen_build_remix_to(Game *g,const Game *source,unsigned *drawn,unsigned target);
#endif
