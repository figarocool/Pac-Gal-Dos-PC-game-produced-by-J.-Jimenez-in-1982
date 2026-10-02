#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "game.h"
#include "audio.h"
#include "speaker.h"
#include "presentation.h"
#include "timing.h"
#include "font.h"
#include "i18n.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifdef VITA
#include <psp2/ctrl.h>
#include <psp2/kernel/clib.h>
#define profile_printf sceClibPrintf
#elif defined(PSP)
#include <pspctrl.h>
#include <pspmoduleinfo.h>
#include <pspthreadman.h>
#define profile_printf printf
#else
#define profile_printf printf
#endif
#ifdef PSP
PSP_MODULE_INFO("PAC-GAL", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
#endif
#ifdef VITA
static void vita_input(int started,int ended,int confirming) {
 static unsigned previous;
 SceCtrlData pad={0};if(sceCtrlPeekBufferPositive(0,&pad,1)<=0)return;
 unsigned held=pad.buttons;
 if(pad.lx<48)held|=SCE_CTRL_LEFT;
 if(pad.lx>208)held|=SCE_CTRL_RIGHT;
 if(pad.ly<48)held|=SCE_CTRL_UP;
 if(pad.ly>208)held|=SCE_CTRL_DOWN;
 unsigned pressed=held&~previous;previous=held;
 const struct {unsigned button;SDL_Keycode key;} keys[]={
  {SCE_CTRL_UP,SDLK_UP},{SCE_CTRL_DOWN,SDLK_DOWN},
  {SCE_CTRL_LEFT,SDLK_LEFT},{SCE_CTRL_RIGHT,SDLK_RIGHT},
  {SCE_CTRL_CROSS,(ended||confirming)?SDLK_y:SDLK_RETURN},
  {SCE_CTRL_CIRCLE,(ended||confirming)?SDLK_n:SDLK_UNKNOWN},
  {SCE_CTRL_START,started?SDLK_SPACE:SDLK_RETURN},
  {SCE_CTRL_SELECT,SDLK_ESCAPE},{SCE_CTRL_LTRIGGER,SDLK_PLUS},
  {SCE_CTRL_RTRIGGER,SDLK_MINUS},{SCE_CTRL_TRIANGLE,SDLK_F11}
 };
 for(unsigned i=0;i<sizeof keys/sizeof *keys;i++)if((pressed&keys[i].button)&&keys[i].key!=SDLK_UNKNOWN) {
  SDL_Event event={0};event.type=SDL_KEYDOWN;event.key.keysym.sym=keys[i].key;SDL_PushEvent(&event);
 }
}
#elif defined(PSP)
static void psp_input(int started,int ended,int confirming) {
 static unsigned previous;
 SceCtrlData pad={0};if(sceCtrlPeekBufferPositive(&pad,1)<=0)return;
 unsigned held=pad.Buttons,pressed=held&~previous;previous=held;
 const struct {unsigned button;SDL_Keycode key;} keys[]={
  {PSP_CTRL_UP,SDLK_UP},{PSP_CTRL_DOWN,SDLK_DOWN},
  {PSP_CTRL_LEFT,SDLK_LEFT},{PSP_CTRL_RIGHT,SDLK_RIGHT},
  {PSP_CTRL_CROSS,(ended||confirming)?SDLK_y:SDLK_RETURN},
  {PSP_CTRL_CIRCLE,(ended||confirming)?SDLK_n:SDLK_UNKNOWN},
  {PSP_CTRL_START,started?SDLK_SPACE:SDLK_RETURN},
  {PSP_CTRL_SELECT,SDLK_ESCAPE}
 };
 for(unsigned i=0;i<sizeof keys/sizeof *keys;i++)if((pressed&keys[i].button)&&keys[i].key!=SDLK_UNKNOWN) {
  SDL_Event event={0};event.type=SDL_KEYDOWN;event.key.keysym.sym=keys[i].key;SDL_PushEvent(&event);
 }
}
#endif
static uint32_t original_seed(void) {
 time_t stamp=time(NULL);struct tm *t=localtime(&stamp);
 /* Original TIME$ conversion uses 360 for hours, not 3600. */
 return t?(uint32_t)(t->tm_hour*360+t->tm_min*60+t->tm_sec):0;
}
static SDL_AudioDeviceID audio_device;
static BasicMusic music;
static Speaker speaker;
static unsigned audio_channels=1;
static void audio_callback(void *userdata,Uint8 *bytes,int length) {
 (void)userdata;
 speaker_s16(&speaker,(int16_t*)bytes,(size_t)length/(sizeof(int16_t)*audio_channels),audio_channels);
}
static void sound(int event) {
 BasicScore score={0};
 if(basic_effect(&music,&score,event)||!audio_device)return;
 SDL_LockAudioDevice(audio_device);
 int result=speaker_enqueue(&speaker,&score);
 SDL_UnlockAudioDevice(audio_device);
 if(result)fprintf(stderr,"Speaker queue full\n");
}
static const unsigned rgb[16]={0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff};
static void raster(const Game *g,uint32_t *pixels,unsigned time) {
 for(int y=0;y<25;y++)for(int x=0;x<80;x++) {
  int a=g->attr[y][x],c=g->ch[y][x];unsigned fg=rgb[a&15],bg=y==24?rgb[0]:rgb[(a>>4)&7];
  if((a&128)&&time%534>=267)fg=bg;
  for(int yy=0;yy<8;yy++)for(int xx=0;xx<8;xx++)
   pixels[(y*8+yy)*640+x*8+xx]=0xff000000|((font[c][yy]&(128>>xx))?fg:bg);
 }
}
static int verify(void) {
 Game build_source,build_target;screen_blank(&build_source);screen_blank(&build_target);
 build_source.ch[0][0]=219;build_source.attr[0][0]=7;build_source.ch[0][1]=249;build_source.attr[0][1]=138;
 unsigned remix_drawn=0;screen_build_remix_to(&build_target,&build_source,&remix_drawn,1);
 assert(remix_drawn==1&&build_target.ch[0][0]==219&&build_target.ch[0][1]==32);
 screen_build_remix_to(&build_target,&build_source,&remix_drawn,screen_build_remix_count());
 assert(remix_drawn==screen_build_remix_count()&&build_target.ch[0][1]==249&&build_target.attr[0][1]==138);
 i18n_set_language("en");assert(!strcmp(i18n_text(I18N_DOTS),"dots")&&i18n_yes_key()=='y');
 i18n_set_language("es");assert(!strcmp(i18n_text(I18N_DOTS),"puntos")&&i18n_yes_key()=='s');
 i18n_set_language("it");assert(!strcmp(i18n_text(I18N_DOTS),"punti")&&i18n_yes_key()=='s');
 i18n_set_language("en");
 assert(game_period_ms(0)==300&&game_period_ms(1000)==350&&game_period_ms(6000)==600&&game_period_ms(30000)==1800);
 const unsigned rates[]={30,60,144};
 for(unsigned k=0;k<3;k++) {
  GameTiming t;game_timing_init(&t,0,1000);unsigned count=0;
  for(unsigned frame=1;frame<=60*rates[k];frame++)
   count+=(unsigned)game_timing_update(&t,(uint64_t)frame*1000/rates[k]);
  assert(count==171&&t.elapsed_ms==60000);
 }
 GameTiming t;game_timing_init(&t,0,1000);
 assert(!game_timing_update(&t,349)&&game_timing_update(&t,350));
 game_timing_pause(&t,500,1);assert(!game_timing_update(&t,10000)&&t.elapsed_ms==500);
 game_timing_pause(&t,10000,0);assert(!game_timing_update(&t,10349));
 assert(game_timing_update(&t,10350)&&t.elapsed_ms==850);
 assert(game_timing_update(&t,30000)&&!game_timing_update(&t,30001));
 game_timing_speed(&t,30001,0);assert(!game_timing_update(&t,30300)&&game_timing_update(&t,30301));
 Game g;game_init(&g,1982);
 int dots=0,power=0;for(int r=0;r<24;r++)for(int c=0;c<80;c++) {
  if(g.ch[r][c]==249){dots++;if(g.attr[r][c]>7)power++;}
 }
 assert(dots==468&&power==10&&g.lives==3);
 unsigned char visited[24][40]={{0}};int qr[960],qc[960],head=0,tail=1,reachable=0;
 qr[0]=18;qc[0]=19;visited[18][19]=1;
 while(head<tail) {
  int r=qr[head],c=qc[head++];if(g.ch[r][c*2]==249)reachable++;
  static const int dy[]={1,-1,0,0},dx[]={0,0,1,-1};
  for(int i=0;i<4;i++) {
   int rr=r+dy[i],cc=c+dx[i];if(rr<0||rr>=24||cc<0||cc>=40)continue;
   if(rr==11&&g.ch[rr][cc*2]==196)cc=39-c;
   int tile=g.ch[rr][cc*2];
   if((tile==32||tile==249)&&!visited[rr][cc]){visited[rr][cc]=1;qr[tail]=rr;qc[tail++]=cc;}
  }
 }
 assert(reachable==468);
 for(uint32_t seed=1;seed<=20;seed++) {
  game_init_remix(&g,seed,seed);assert(g.remix&&g.level==(int)seed&&g.dots>400&&g.dots<700&&g.lives==3);
  assert(g.ch[24][79]==32&&g.ch[24][17]==32&&g.ch[24][26]==32);
  for(int c=14;c<17;c++)assert(g.attr[24][c]==7);
  int powers=0;for(int r=0;r<24;r++)for(int c=0;c<40;c++)
   if(g.ch[r][c*2]==249&&g.attr[r][c*2]==138)powers++;
  assert(powers==4&&g.ch[18][38]==1);
  assert(g.ch[11][32]==201&&g.ch[11][46]==187);
  assert(g.ch[11][33]==205&&g.ch[11][35]==205&&g.ch[11][37]==205);
  assert(g.ch[11][43]==205&&g.ch[11][45]==205);
  assert(g.ch[12][32]==186&&g.ch[14][46]==186);
  assert(g.ch[12][33]==32&&g.ch[14][47]==32);
  assert(g.ch[15][32]==200&&g.ch[15][46]==188);
  assert(g.ch[15][47]==32);
  assert(g.ch[11][39]==32&&g.ch[11][41]==32);
  for(int r=13;r<=15;r++)for(int c=17;c<=22;c++) {
   int ch=g.ch[r-1][c*2];
   (void)ch;
   assert(ch==32||(r==14&&c>=18&&c<=21&&ch==c-15));
  }
  for(int r=8;r<=10;r++)for(int c=19;c<=20;c++)
   assert(g.ch[r][c*2]==32||g.ch[r][c*2]==249);
  assert(g.ch[11][38]==32&&g.ch[11][40]==32);
  g.player=(Actor){3,2,0,0,32,7,0};g.ch[2][4]=32;g.ch[2][6]=249;g.attr[2][6]=138;
  game_direction(&g,0,1);game_player_step(&g);
  assert(g.player.col==3&&g.sound==SOUND_DOT);
  for(int i=0;i<4;i++)assert(g.ghost[i].color==26&&g.ghost[i].timer>0);
  game_init_remix(&g,seed,seed);
  assert(g.tunnel_left_row>=3&&g.tunnel_left_row<=23&&(g.tunnel_left_row&1));
  assert(g.tunnel_right_row>=3&&g.tunnel_right_row<=23&&(g.tunnel_right_row&1));
  assert(g.tunnel_left_row!=g.tunnel_right_row);
  for(int c=1;c<39;c++)assert(g.ch[1][c*2]==32||g.ch[1][c*2]==249);
  for(int r=3;r<=23;r++)assert(g.ch[r-1][76]==32||g.ch[r-1][76]==249);
  for(int c=0;c<40;c++)assert(g.ch[0][c*2]==220&&g.ch[23][c*2]==223);
  for(int r=0;r<24;r++) {
   int edge=r==0?220:r==23?223:219;
   (void)edge;
   assert(g.ch[r][0]==(r==g.tunnel_left_row-1?196:edge));
   assert(g.ch[r][78]==(r==g.tunnel_right_row-1?196:edge));
   assert(g.ch[r][1]==(r==0?220:r==23?223:32));
   assert(g.ch[r][79]==32);
  }
  for(int r=12;r<=14;r++)for(int c=35;c<=45;c+=2)assert(g.ch[r][c]==32);
  unsigned char seen[24][40]={{0}};int rr[960],cc[960],h=0,n=1;
  rr[0]=18;cc[0]=19;seen[18][19]=1;
  while(h<n){int r=rr[h],c=cc[h++];static const int dy[]={1,-1,0,0},dx[]={0,0,1,-1};
   for(int d=0;d<4;d++){int y=r+dy[d],x=c+dx[d];if(y<0||y>=24||x<0||x>=40)continue;
    int t=g.ch[y][x*2];
    if(y==g.tunnel_left_row-1&&x==0){y=g.tunnel_right_row-1;x=38;t=g.ch[y][x*2];}
    else if(y==g.tunnel_right_row-1&&x==39){y=g.tunnel_left_row-1;x=1;t=g.ch[y][x*2];}
    if((t==32||t==249||t==1||(t>=3&&t<=6))&&!seen[y][x]){seen[y][x]=1;rr[n]=y;cc[n++]=x;}
   }
  }
  int reachable_dots=0;for(int r=0;r<24;r++)for(int c=0;c<40;c++) {
   int tile=g.ch[r][c*2],walkable=tile==32||tile==249||tile==1||(tile>=3&&tile<=6);
   if(walkable)assert(seen[r][c]);
   if(seen[r][c]&&(tile==249||(r==18&&c==19&&g.player.under==249)))reachable_dots++;
  }
  assert(reachable_dots==g.dots);
  g.player=(Actor){g.tunnel_left_row,1,0,0,g.ch[g.tunnel_left_row-1][2],7,0};
  g.ch[g.player.row-1][2]=1;game_direction(&g,0,-1);game_player_step(&g);
  assert(g.player.row==g.tunnel_right_row&&g.player.col==38);
  g.ch[g.player.row-1][76]=1;g.player=(Actor){g.tunnel_right_row,38,0,0,32,7,0};
  game_direction(&g,0,1);game_player_step(&g);
  assert(g.player.row==g.tunnel_left_row&&g.player.col==1);
  game_init_remix(&g,seed,seed);g.aggression=1.0f;
  g.ch[g.tunnel_left_row-1][2]=3;
  g.ghost[0]=(Actor){g.tunnel_left_row,1,0,-1,32,7,1};
  game_ghost_step(&g);
  assert(g.ghost[0].row==g.tunnel_right_row&&g.ghost[0].col==38&&g.ghost[0].dx==-1);
  game_init_remix(&g,seed,seed);
  g.aggression=1.0f;
  for(int turn=0;turn<1000&&!g.ended;turn++) {
   game_ghost_step(&g);
   for(int i=0;i<4;i++){Actor *a=&g.ghost[i];if(a->row<1||a->row>24||a->col<0||a->col>=40||(a->under!=32&&a->under!=249))return 1;}
  }
 }
 game_init(&g,1982);
 game_direction(&g,0,0);game_player_step(&g);
 assert(g.event_count==1&&g.events[0]==SOUND_FLOOR);
 game_init(&g,1);g.player.row=6;g.player.col=2;game_direction(&g,0,-1);game_player_step(&g);
 assert(g.event_count==2&&g.events[0]==SOUND_POWER&&g.events[1]==SOUND_DOT);
 game_init(&g,1982);
 /* Wall, eating, tunnel and collision branches, with ghosts out of the way. */
 game_direction(&g,0,-1);game_player_step(&g);assert(g.player.col==18&&g.dots==467);
 game_init(&g,1);g.player.row=2;g.player.col=1;game_direction(&g,-1,0);
 game_player_step(&g);assert(g.player.row==2&&g.dots==467);
 game_direction(&g,1,0);game_player_step(&g);assert(g.dots==466&&g.player.row==3);
 game_init(&g,1);g.player.row=12;g.player.col=2;game_direction(&g,0,-1);
 game_player_step(&g);assert(g.player.col==37&&g.player.row==12);
 game_init(&g,1);g.player.row=6;g.player.col=2;game_direction(&g,0,-1);
 game_player_step(&g);assert(g.dots==467&&g.ghost[0].color==26&&g.ghost[0].timer==13);
 game_init(&g,1);g.ch[18][36]=3;g.attr[18][36]=7;game_direction(&g,0,-1);
 game_player_step(&g);assert(g.lives==2&&g.player.col==19);
 game_init(&g,1);g.lives=2;g.ghost[0].row=19;g.ghost[0].col=18;
 g.ch[18][36]=3;g.attr[18][36]=138;game_direction(&g,0,-1);
 game_player_step(&g);assert(g.lives==3&&g.ghost[0].row==14&&g.player.col==18);
 game_init(&g,1);g.dots=1;game_direction(&g,0,-1);game_player_step(&g);assert(g.ended==1);
 for(uint32_t seed=1;seed<=20;seed++) {
  game_init(&g,seed);
  for(int t=0;t<5000&&!g.ended;t++) {
   static const int dy[]={-1,0,1,0},dx[]={0,1,0,-1};int dir=(t/11)%4;
   game_direction(&g,dy[dir],dx[dir]);game_player_step(&g);game_ghost_step(&g);
   assert(g.dots>=0&&g.dots<=468&&g.lives>=1&&g.lives<=3);
   for(int i=0;i<4;i++)assert(g.ghost[i].row>=1&&g.ghost[i].row<=24&&g.ghost[i].col>=0&&g.ghost[i].col<40);
  }
 }
 puts("OK: timing at 30/60/144 FPS, pause, stalls, speed changes; original maze, 468 dots, 10 special dots, walls, tunnel, power, deaths, eating, victory, simulation.");return 0;
}
int main(int argc,char **argv) {
 int speed=1000,start=0,remix_start=0,frames=0,timing_report=0,performance_report=0,fixed_seed=0;uint32_t seed=original_seed();const char *shot=NULL;
 for(int i=1;i<argc;i++) {
  if(!strcmp(argv[i],"--self-test"))return verify();
  if(!strcmp(argv[i],"--performance-report"))performance_report=1;
  else if(!strcmp(argv[i],"--timing-report"))timing_report=1;
  else if(!strcmp(argv[i],"--remix")){start=1;remix_start=1;}
  else if(!strcmp(argv[i],"--start"))start=1;
  else if(!strcmp(argv[i],"--speed")&&i+1<argc)speed=atoi(argv[++i]);
  else if(!strcmp(argv[i],"--seed")&&i+1<argc){seed=(uint32_t)strtoul(argv[++i],NULL,10);fixed_seed=1;}
  else if(!strcmp(argv[i],"--frames")&&i+1<argc)frames=atoi(argv[++i]);
  else if(!strcmp(argv[i],"--screenshot")&&i+1<argc)shot=argv[++i];
  else if(!strcmp(argv[i],"--help")){puts("pac-gal [--start|--remix] [--speed 0..30000] [--seed N] [--frames N] [--screenshot file.bmp] [--self-test] [--timing-report]");return 0;}
  else if(strcmp(argv[i],"--start")){fprintf(stderr,"Unknown argument: %s\n",argv[i]);return 1;}
 }
 if(speed<0)speed=0;
 if(speed>30000)speed=30000;
 #ifdef VITA
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 #elif defined(PSP)
 sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 #endif
 basic_music_init(&music);SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0){fprintf(stderr,"SDL: %s\n",SDL_GetError());return 1;}i18n_init();
 SDL_AudioSpec wanted={0},got;
 #ifdef VITA
 wanted.freq=48000;audio_channels=2;
 #elif defined(PSP)
 wanted.freq=44100;audio_channels=1;
 #else
 wanted.freq=44100;audio_channels=1;
 #endif
 wanted.format=AUDIO_S16SYS;wanted.channels=(Uint8)audio_channels;wanted.samples=512;wanted.callback=audio_callback;
 speaker_video_init(&speaker,(unsigned)wanted.freq,.15f);
 audio_device=SDL_OpenAudioDevice(NULL,0,&wanted,&got,0);
 if(audio_device)SDL_PauseAudioDevice(audio_device,0);
 else fprintf(stderr,"Audio: %s\n",SDL_GetError());
 #ifdef VITA
 const int window_height=544;const unsigned window_flags=SDL_WINDOW_FULLSCREEN;
 #elif defined(PSP)
 const int window_height=272;const unsigned window_flags=SDL_WINDOW_FULLSCREEN;
 #else
 const int window_height=720;const unsigned window_flags=SDL_WINDOW_RESIZABLE;
 #endif
 SDL_Window *w=SDL_CreateWindow(i18n_text(I18N_WINDOW_TITLE),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,window_height,window_flags);
 if(!w){fprintf(stderr,"Window: %s\n",SDL_GetError());SDL_Quit();return 1;}
 SDL_Renderer *renderer=SDL_CreateRenderer(w,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
 if(!renderer)renderer=SDL_CreateRenderer(w,-1,SDL_RENDERER_SOFTWARE);
 if(!renderer){fprintf(stderr,"Renderer: %s\n",SDL_GetError());SDL_Quit();return 1;}
 #ifdef VITA
 SDL_RenderSetLogicalSize(renderer,960,544);
 #elif defined(PSP)
 SDL_RenderSetLogicalSize(renderer,480,272);
 #endif
 SDL_RendererInfo render_info={0};SDL_GetRendererInfo(renderer,&render_info);
 SDL_Texture *tex=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,640,200);
 if(!tex){fprintf(stderr,"Texture: %s\n",SDL_GetError());SDL_Quit();return 1;}
 static uint32_t pixels[640*200];Game game,display;game_init(&game,seed);
 int selected_mode=remix_start?1:0,paused=0,running=1,fullscreen=0,ticks=0,confirm_exit=0,confirm_resume_paused=0,confirm_from_menu=0;
 if(remix_start)game_init_remix(&game,seed,1);
 uint64_t remix_total_dots=0;
 #ifdef VITA
 fullscreen=1;
 #elif defined(PSP)
 fullscreen=1;
 #endif
 GameTiming timing;game_timing_init(&timing,SDL_GetTicks64(),speed);
 uint64_t epoch=0;unsigned steps=0;int shown_end=0;uint64_t remix_next_at=0;
 int intro=0;unsigned intro_drawn=0,build_drawn=0,pending_rng_low=5;uint64_t intro_at=0,end_at=0;uint64_t effect_until=0;Game opening;screen_blank(&opening);
 if(remix_start){intro=1;intro_at=SDL_GetTicks64();sound(SOUND_INTRO);}
 uint64_t profile_start=SDL_GetTicks64(),last_frame=profile_start,max_frame=0;unsigned frame_count=0,stalls=0;
 while(running) {
  if(performance_report){uint64_t stamp=SDL_GetTicks64(),dt=stamp-last_frame;last_frame=stamp;if(dt>max_frame)max_frame=dt;if(dt>50)stalls++;frame_count++;}
  #ifdef VITA
  /* The menu must remain actionable even if the previous game ended. */
  vita_input(start,game.ended&&shown_end==2,confirm_exit);
  #elif defined(PSP)
  psp_input(start,game.ended&&shown_end==2,confirm_exit);
  #endif
  SDL_Event e;while(SDL_PollEvent(&e)) {
   if(e.type==SDL_QUIT)running=0;
   if(e.type!=SDL_KEYDOWN)continue;
   SDL_Keycode k=e.key.keysym.sym;
  if(k==SDLK_ESCAPE){
   if(confirm_exit){confirm_exit=0;if(!confirm_from_menu)game_timing_pause(&timing,SDL_GetTicks64(),confirm_resume_paused);continue;}
   confirm_from_menu=!start;confirm_resume_paused=paused||effect_until!=0;confirm_exit=1;
   if(start)game_timing_pause(&timing,SDL_GetTicks64(),1);
   continue;
  }
   if(k==SDLK_F11){fullscreen=!fullscreen;
    #if defined(VITA) || defined(PSP)
    SDL_RenderSetLogicalSize(renderer,fullscreen?960:640,fullscreen?544:480);
    #else
    SDL_SetWindowFullscreen(w,fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:0);
    #endif
   }
   if(confirm_exit) {
    if(k==SDLK_y||k==SDLK_s){confirm_exit=0;if(confirm_from_menu)running=0;else {start=0;paused=0;intro=0;effect_until=0;shown_end=0;end_at=0;remix_next_at=0;confirm_from_menu=0;confirm_resume_paused=0;game.ended=0;game.effect=0;} }
    else if(k==SDLK_n){confirm_exit=0;if(!confirm_from_menu)game_timing_pause(&timing,SDL_GetTicks64(),confirm_resume_paused);confirm_from_menu=0;}
    continue;
   }
   if(!start) {
    if(k==SDLK_UP||k==SDLK_w||k==SDLK_LEFT)selected_mode=0;
    if(k==SDLK_DOWN||k==SDLK_s||k==SDLK_RIGHT)selected_mode=1;
    if(k==SDLK_RETURN||k==SDLK_KP_ENTER||k==SDLK_SPACE){
     start=1;paused=0;shown_end=0;intro_drawn=0;epoch=0;steps=0;
     if(selected_mode==0){game_init(&game,seed);intro=1;intro_at=SDL_GetTicks64();screen_blank(&opening);sound(SOUND_INTRO);}
     else {remix_total_dots=0;game_init_remix(&game,seed,1);intro=1;intro_at=SDL_GetTicks64();intro_drawn=build_drawn=0;screen_blank(&opening);sound(SOUND_INTRO);}
    }
    continue;
   }
   if(k==SDLK_r||(game.ended&&shown_end==2&&(k==SDLK_y||k==SDLK_s))){pending_rng_low=game.rng&255u;if(!fixed_seed)seed=original_seed();screen_blank(&opening);memcpy(opening.ch[24],game.ch[24],80);memcpy(opening.attr[24],game.attr[24],80);if(game.remix){remix_total_dots=0;game_init_remix(&game,seed,(unsigned)game.level);}else game_restart(&game,seed);effect_until=0;paused=0;shown_end=0;remix_next_at=0;game_timing_init(&timing,SDL_GetTicks64(),speed);epoch=0;steps=0;intro=game.remix?0:1;intro_drawn=0;intro_at=SDL_GetTicks64();if(!game.remix)sound(SOUND_INTRO);}
   if(game.ended&&shown_end==2&&k==SDLK_n){screen_blank(&game);screen_text(&game,0,0,i18n_text(I18N_GOODBYE),7);display=game;raster(&display,pixels,SDL_GetTicks());SDL_UpdateTexture(tex,NULL,pixels,640*4);SDL_RenderClear(renderer);SDL_RenderCopy(renderer,tex,NULL,NULL);SDL_RenderPresent(renderer);running=0;continue;}
   if(intro||effect_until||game.ended)continue;
   if(k==SDLK_SPACE||k==SDLK_p){paused=!paused;game_timing_pause(&timing,SDL_GetTicks64(),paused);}
   if(game.ended==1&&game.remix)continue;
   if(k==SDLK_UP||k==SDLK_w)game_direction(&game,-1,0);
   if(k==SDLK_DOWN||k==SDLK_s)game_direction(&game,1,0);
   if(k==SDLK_LEFT||k==SDLK_a)game_direction(&game,0,-1);
   if(k==SDLK_RIGHT||k==SDLK_d)game_direction(&game,0,1);
   if(k==SDLK_PLUS||k==SDLK_EQUALS||k==SDLK_KP_PLUS){speed-=1000;if(speed<0)speed=0;game_timing_speed(&timing,SDL_GetTicks64(),speed);}
   if(k==SDLK_MINUS||k==SDLK_KP_MINUS){speed+=1000;if(speed>30000)speed=30000;game_timing_speed(&timing,SDL_GetTicks64(),speed);}
  }
  Uint64 now=SDL_GetTicks64();
  if(intro==1) {
   uint64_t elapsed=now-intro_at;
   unsigned target=elapsed<165?(unsigned)(elapsed*60/165):60+(unsigned)((elapsed-165)*60/769);
   if(target>120)target=120;
   while(intro_drawn<target){if(intro_drawn==60)sound(SOUND_INTRO);screen_intro_frame(&opening,intro_drawn++);}
   if(target==120){intro=2;intro_at=now;build_drawn=0;memset(opening.ch,32,24*80);memset(opening.attr,7,24*80);}
  } else if(intro==2) {
   uint64_t elapsed=now-intro_at;unsigned count=game.remix?screen_build_remix_count():screen_build_count();
   unsigned target=elapsed>=1208?count:(unsigned)(elapsed*count/1208);
   if(game.remix)screen_build_remix_to(&opening,&game,&build_drawn,target);
   else screen_build_to(&opening,&build_drawn,target);
   if(build_drawn==count){if(!fixed_seed&&!game.remix){seed=original_seed();game.rng=pending_rng_low;game_restart(&game,seed);}intro=0;game_timing_init(&timing,now,speed);}
  }
  if(effect_until&&now>=effect_until){effect_until=0;game.effect=0;game_timing_pause(&timing,now,paused);}
  if(!confirm_exit&&game.ended==1&&game.remix&&!remix_next_at)remix_next_at=now+1800;
  if(!confirm_exit&&remix_next_at&&now>=remix_next_at){unsigned level=(unsigned)game.level+1;seed=seed*1664525u+1013904223u;game_init_remix(&game,seed,level);shown_end=0;remix_next_at=0;epoch=0;steps=0;game_timing_init(&timing,now,speed);}
  if(!confirm_exit&&start&&!intro&&!effect_until&&!game.ended&&game_timing_update(&timing,now)) {
   steps++;game.sound=0;game.event_count=0;game.effect=0;game_player_step(&game);
   if(game.remix&&game.sound==SOUND_DOT&&remix_total_dots<UINT64_MAX)remix_total_dots++;
   if(game.sound==3||game.sound==4)epoch=timing.elapsed_ms;
   unsigned rounds=1+(unsigned)((timing.elapsed_ms-epoch)/180000);
   for(unsigned j=0;j<rounds&&!game.ended;j++)game_ghost_step(&game);
   if(game.sound==3||game.sound==4)epoch=timing.elapsed_ms;
   for(unsigned j=0;j<game.event_count;j++)sound(game.events[j]);
   if(game.effect){effect_until=now+(game.effect==SOUND_DEATH?330:165);game_timing_pause(&timing,now,1);}
  }
  if(game.ended&&!effect_until&&!shown_end){shown_end=1;end_at=now;}
  if(shown_end==1&&(game.ended==2||now-end_at>=15000)){shown_end=2;game_replay_prompt(&game);}
  display=game;if(!start)screen_mode(&display,selected_mode);
  else if(intro)display=opening;
  else if(effect_until){memcpy(display.ch,game.effect_ch,sizeof display.ch);memcpy(display.attr,game.effect_attr,sizeof display.attr);}
  else if(paused) {
   #if defined(VITA) || defined(PSP)
   screen_text(&display,24,24,i18n_text(I18N_PAUSE_VITA),15);
   #else
   screen_text(&display,24,24,i18n_text(I18N_PAUSE_PC),15);
   #endif
  }
  if(confirm_exit) {
   #if defined(VITA) || defined(PSP)
   screen_text(&display,22,23,i18n_text(I18N_EXIT_VITA),15);
   #else
   screen_text(&display,22,24,i18n_text(I18N_EXIT_PC),15);
   #endif
  }
  if(start&&game.remix&&!intro){
   const char logo[]="    P A C - G A L    ";const char *byline=i18n_text(I18N_REMIX_CREDIT);char total[16];
   if(remix_total_dots<10000000)snprintf(total,sizeof total,"T:%llu",(unsigned long long)remix_total_dots);
   else snprintf(total,sizeof total,"T:%.1e",(double)remix_total_dots);
   screen_text(&display,24,18,total,7);
   for(unsigned c=0;c<sizeof logo-1;c++){display.ch[24][27+c]=(unsigned char)logo[c];display.attr[24][27+c]=(unsigned char)(c<18?15:7);}
   screen_text(&display,24,49,byline,7);
   display.attr[24][79]=7;
  }
  raster(&display,pixels,now);SDL_UpdateTexture(tex,NULL,pixels,640*4);
  SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
  SDL_Rect view;
  #ifdef VITA
  view=(SDL_Rect){0,0,960,544};
  #elif defined(PSP)
  view=(SDL_Rect){0,0,480,272};
  #else
  SDL_GetRendererOutputSize(renderer,&view.w,&view.h);view.x=view.y=0;
  #endif
  SDL_RenderCopy(renderer,tex,NULL,&view);SDL_RenderPresent(renderer);
  if(frames&&++ticks>=frames)running=0;
  if(!(render_info.flags&SDL_RENDERER_PRESENTVSYNC))SDL_Delay(1);
 }
 if(shot) {
  SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels,640,200,32,640*4,SDL_PIXELFORMAT_ARGB8888);
  if(!surface||SDL_SaveBMP(surface,shot)<0){fprintf(stderr,"Screenshot: %s\n",SDL_GetError());return 1;}
  SDL_FreeSurface(surface);
 }
 #ifdef VITA
 if(performance_report) {
  FILE *report=fopen("ux0:/data/pacgal-performance.txt","w");
  if(report) {
   fprintf(report,"Performance: %u frames in %llu ms; max frame %llu ms; stalls >50 ms: %u\n",frame_count,(unsigned long long)(SDL_GetTicks64()-profile_start),(unsigned long long)max_frame,stalls);
   fprintf(report,"Timing: %u moves in %llu ms of active play, target %u ms/move\n",steps,(unsigned long long)timing.elapsed_ms,timing.period_ms);fclose(report);
  }
 }
 #endif
 if(performance_report)profile_printf("Performance: %u frames in %llu ms; max frame %llu ms; stalls >50 ms: %u\n",frame_count,(unsigned long long)(SDL_GetTicks64()-profile_start),(unsigned long long)max_frame,stalls);
 if(timing_report)profile_printf("Timing: %u moves in %llu ms of active play, target %u ms/move\n",steps,(unsigned long long)timing.elapsed_ms,timing.period_ms);
 SDL_CloseAudioDevice(audio_device);SDL_DestroyTexture(tex);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
