#include "presentation.h"
#include <string.h>
#include <stdio.h>
#include "build.h"
#include "i18n.h"
void screen_text(Game *g,int row,int col,const char *s,int attr) {
 for(;*s&&col<80;col++,s++){g->ch[row][col]=(unsigned char)*s;g->attr[row][col]=(unsigned char)attr;}
}
void screen_blank(Game *g) {
 memset(g,0,sizeof *g);memset(g->ch,32,24*80);memset(g->attr,7,sizeof g->attr);
}
static void centered(Game *g,int row,const char *s,int attr) {
 screen_text(g,row,(80-(int)strlen(s))/2,s,attr);
}
void screen_speed(Game *g,const char *speed) {
 screen_blank(g);
 centered(g,8,"P A C - G A L",15);
 char line[64];snprintf(line,sizeof line,i18n_text(I18N_SPEED_LABEL),speed);
 centered(g,11,line,15);
 #if defined(VITA) || defined(PSP)
 centered(g,14,i18n_text(I18N_MENU_HELP_VITA),7);
 centered(g,18,i18n_text(I18N_SPEED_HELP_VITA),7);
 centered(g,20,i18n_text(I18N_SCREEN_HELP_VITA),7);
 #else
 centered(g,14,i18n_text(I18N_SPEED_START),7);
 #endif
 centered(g,16,i18n_text(I18N_SPEED_HINT),7);
}
void screen_mode(Game *g,int selected) {
 screen_blank(g);centered(g,6,"P A C - G A L",15);
 centered(g,9,i18n_text(I18N_MODE_HEADING),7);
 char normal[32],remix[32];snprintf(normal,sizeof normal,"%s %s",selected==0?">":" ",i18n_text(I18N_NORMAL));
 snprintf(remix,sizeof remix,"%s %s",selected==1?">":" ",i18n_text(I18N_REMIX));
 centered(g,12,normal,selected==0?15:7);
 centered(g,14,remix,selected==1?15:7);
 centered(g,17,i18n_text(I18N_NORMAL_DESC),7);
 centered(g,19,i18n_text(I18N_REMIX_DESC),7);
 #if defined(VITA) || defined(PSP)
 centered(g,22,i18n_text(I18N_MENU_HELP_VITA),7);
 #else
 centered(g,22,i18n_text(I18N_MENU_HELP_PC),7);
 #endif
}
void screen_intro_frame(Game *g,unsigned frame) {
 unsigned stage=frame/60,pos=frame%60;
 if(!stage){int col=59-(int)pos;screen_text(g,13,col,"\2 ",7);}
 else {screen_text(g,13,(int)pos," \3   \4\5\6       \2",7);}
}
unsigned screen_build_count(void) {return sizeof original_draw/sizeof *original_draw;}
void screen_build_to(Game *g,unsigned *drawn,unsigned target) {
 unsigned count=screen_build_count();if(target>count)target=count;
 while(*drawn<target) {
  unsigned i=(*drawn)++,r=original_draw[i].cell/80,c=original_draw[i].cell%80;
  g->ch[r][c]=original_draw[i].ch;g->attr[r][c]=original_draw[i].attr;
 }
}
unsigned screen_build_remix_count(void) {return 24u*80u;}
void screen_build_remix_to(Game *g,const Game *source,unsigned *drawn,unsigned target) {
 unsigned count=screen_build_remix_count();if(target>count)target=count;
 while(*drawn<target) {
  unsigned i=(*drawn)++,r=i/80,c=i%80;
  g->ch[r][c]=source->ch[r][c];g->attr[r][c]=source->attr[r][c];
 }
}
