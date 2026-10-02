#include "i18n.h"
#include <string.h>
#include <stdlib.h>
#ifdef PACGAL_DOS
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#ifdef VITA
#include <psp2/apputil.h>
#include <psp2/system_param.h>
#elif defined(PSP)
#include <psputility_sysparam.h>
#endif

enum { LANG_EN, LANG_ES, LANG_IT };
static int active_language=LANG_EN;
static const char *const text[3][I18N_COUNT]={
 {
  [I18N_DOTS]="dots",[I18N_VICTORY]="You did it!!!",
  [I18N_REPLAY_PC]="Play again? Y: YES  N: NO",[I18N_REPLAY_VITA]="Play again? X: YES  O: NO",
  [I18N_MODE_HEADING]="CHOOSE MODE",[I18N_NORMAL]="NORMAL",[I18N_REMIX]="REMIX",
  [I18N_NORMAL_DESC]="NORMAL: original game and classic levels",[I18N_REMIX_DESC]="REMIX: endless random mazes",
  [I18N_MENU_HELP_PC]="ARROWS: SELECT    ENTER: CONFIRM",[I18N_MENU_HELP_VITA]="UP/DOWN: SELECT    X: CONFIRM",
  [I18N_PAUSE_PC]="PAUSED - SPACE TO RESUME",[I18N_PAUSE_VITA]="PAUSED - START TO RESUME",
  [I18N_EXIT_PC]="EXIT? Y: YES  N: NO",[I18N_EXIT_VITA]="EXIT? X: YES  O: NO",
  [I18N_WINDOW_TITLE]="PAC-GAL 1982 | Arrows: move | Space: pause | Esc: menu",
  [I18N_GOODBYE]="Thanks for playing!",[I18N_SPEED_LABEL]="Speed (0-30000): %s",
  [I18N_SPEED_START]="Press ENTER to start",[I18N_SPEED_HINT]="A higher number slows the game",
  [I18N_SPEED_HELP_PC]="ARROWS: SELECT    ENTER: CONFIRM",[I18N_SPEED_HELP_VITA]="START: PAUSE    SELECT: MENU",
  [I18N_SCREEN_HELP_VITA]="TRIANGLE: SCREEN SIZE",
  [I18N_REMIX_CREDIT]="by Stefano Basile, October 2026"
  ,[I18N_LEVEL_FMT]="LEVEL %u"
 },
 {
  [I18N_DOTS]="puntos",[I18N_VICTORY]="Lo lograste!!!",
  [I18N_REPLAY_PC]="Jugar otra vez? S: SI  N: NO",[I18N_REPLAY_VITA]="Jugar otra vez? X: SI  O: NO",
  [I18N_MODE_HEADING]="ELIGE EL MODO",[I18N_NORMAL]="NORMAL",[I18N_REMIX]="REMIX",
  [I18N_NORMAL_DESC]="NORMAL: juego original y niveles clasicos",[I18N_REMIX_DESC]="REMIX: laberintos aleatorios infinitos",
  [I18N_MENU_HELP_PC]="FLECHAS: ELEGIR    ENTER: CONFIRMAR",[I18N_MENU_HELP_VITA]="ARRIBA/ABAJO: ELEGIR    X: CONFIRMAR",
  [I18N_PAUSE_PC]="PAUSA - ESPACIO PARA SEGUIR",[I18N_PAUSE_VITA]="PAUSA - START PARA SEGUIR",
  [I18N_EXIT_PC]="SALIR? S: SI  N: NO",[I18N_EXIT_VITA]="SALIR? X: SI  O: NO",
  [I18N_WINDOW_TITLE]="PAC-GAL 1982 | Flechas: mover | Espacio: pausa | Esc: menu",
  [I18N_GOODBYE]="Gracias por jugar!",[I18N_SPEED_LABEL]="Velocidad (0-30000): %s",
  [I18N_SPEED_START]="Pulsa ENTER para empezar",[I18N_SPEED_HINT]="Un numero mayor ralentiza el juego",
  [I18N_SPEED_HELP_PC]="FLECHAS: ELEGIR    ENTER: CONFIRMAR",[I18N_SPEED_HELP_VITA]="START: PAUSA    SELECT: MENU",
  [I18N_SCREEN_HELP_VITA]="TRIANGULO: TAMANO PANTALLA",
  [I18N_REMIX_CREDIT]="por Stefano Basile, Oct. 2026"
  ,[I18N_LEVEL_FMT]="NIVEL %u"
 },
 {
  [I18N_DOTS]="punti",[I18N_VICTORY]="Ce l'hai fatta!!!",
  [I18N_REPLAY_PC]="Ancora? S: SI  N: NO",[I18N_REPLAY_VITA]="Ancora? X: SI  O: NO",
  [I18N_MODE_HEADING]="SCEGLI LA MODALITA",[I18N_NORMAL]="NORMALE",[I18N_REMIX]="REMIX",
  [I18N_NORMAL_DESC]="NORMALE: gioco originale e livelli classici",[I18N_REMIX_DESC]="REMIX: labirinti casuali senza fine",
  [I18N_MENU_HELP_PC]="FRECCE: SCEGLI    INVIO: CONFERMA",[I18N_MENU_HELP_VITA]="SU/GIU: SCEGLI    X: CONFERMA",
  [I18N_PAUSE_PC]="PAUSA - SPAZIO PER CONTINUARE",[I18N_PAUSE_VITA]="PAUSA - START PER CONTINUARE",
  [I18N_EXIT_PC]="USCIRE? S: SI  N: NO",[I18N_EXIT_VITA]="USCIRE? X: SI  O: NO",
  [I18N_WINDOW_TITLE]="PAC-GAL 1982 | Frecce: muovi | Spazio: pausa | Esc: menu",
  [I18N_GOODBYE]="Grazie per aver giocato!",[I18N_SPEED_LABEL]="Velocita (0-30000): %s",
  [I18N_SPEED_START]="Premi INVIO per iniziare",[I18N_SPEED_HINT]="Un numero maggiore rallenta il gioco",
  [I18N_SPEED_HELP_PC]="FRECCE: SCEGLI    INVIO: CONFERMA",[I18N_SPEED_HELP_VITA]="START: PAUSA    SELECT: MENU",
  [I18N_SCREEN_HELP_VITA]="TRIANGOLO: DIMENSIONE SCHERMO",
  [I18N_REMIX_CREDIT]="di Stefano Basile, Ottobre 2026"
  ,[I18N_LEVEL_FMT]="LIVELLO %u"
 }
};

static int language_id(const char *language) {
 if(!language)return -1;
 if(!strncmp(language,"it",2))return LANG_IT;
 if(!strncmp(language,"es",2))return LANG_ES;
 if(!strncmp(language,"en",2))return LANG_EN;
 return -1;
}
void i18n_set_language(const char *language) {
 int found=language_id(language);if(found>=0)active_language=found;
}
void i18n_init(void) {
#ifdef VITA
 SceAppUtilInitParam init={0};SceAppUtilBootParam boot={0};int lang=-1;
 int initialized=sceAppUtilInit(&init,&boot)>=0;
 if(sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG,&lang)>=0) {
  if(lang==SCE_SYSTEM_PARAM_LANG_ITALIAN)active_language=LANG_IT;
  else if(lang==SCE_SYSTEM_PARAM_LANG_SPANISH)active_language=LANG_ES;
  else active_language=LANG_EN;
 }
 if(initialized)sceAppUtilShutdown();
#elif defined(PSP)
 int lang=-1;
 if(sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&lang)>=0) {
  if(lang==PSP_SYSTEMPARAM_LANGUAGE_ITALIAN)active_language=LANG_IT;
  else if(lang==PSP_SYSTEMPARAM_LANGUAGE_SPANISH)active_language=LANG_ES;
  else active_language=LANG_EN;
 }
#else
#ifdef PACGAL_DOS
 int locale_count=0;SDL_Locale **locales=SDL_GetPreferredLocales(&locale_count);
 if(locales) {
  for(int i=0;i<locale_count;i++) {
   int found=language_id(locales[i]->language);if(found>=0){active_language=found;break;}
  }
  SDL_free(locales);
 }
#else
 SDL_Locale *locales=SDL_GetPreferredLocales();
 if(locales) {
  for(SDL_Locale *locale=locales;locale->language;locale++) {
   int found=language_id(locale->language);if(found>=0){active_language=found;break;}
  }
  SDL_free(locales);
 }
#endif
 if(active_language==LANG_EN) {
  const char *environment=getenv("LC_ALL");if(!environment||!*environment)environment=getenv("LC_MESSAGES");
  if(!environment||!*environment)environment=getenv("LANG");
  int found=language_id(environment);if(found>=0)active_language=found;
 }
#endif
}
const char *i18n_text(I18nKey key) {
 if((unsigned)key>=I18N_COUNT)return "";
 return text[active_language][key];
}
char i18n_yes_key(void) {return active_language==LANG_EN?'y':'s';}
