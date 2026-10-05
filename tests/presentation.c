#include "presentation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
 if(argc!=2)return 2;
 Game g;screen_blank(&g);unsigned n=(unsigned)atoi(argv[1]);
 if(!strcmp(argv[1],"build")){unsigned drawn=0;screen_build_to(&g,&drawn,screen_build_count());}
 else for(unsigned i=0;i<n;i++)screen_intro_frame(&g,i);
 for(int r=0;r<25;r++)for(int c=0;c<80;c++){putchar(g.ch[r][c]);putchar(g.attr[r][c]);}
 return 0;
}
