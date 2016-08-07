%{
#include <stdio.h>
#include "y.tab.h"
%}
number [0-9]+ 
%%
{number}           	yylval=atoi(yytext);  return NUMBER;
heat                    return TOKHEAT;
on|off                  yylval=!strcmp(yytext,"on");return STATE;
target                  return TOKTARGET;
temperature             return TOKTEMPERATURE;
\n                      /* ignore end of line */;
[ \t]+                  /* ignore whitespace */;
%%