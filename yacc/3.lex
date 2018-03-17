%{
#include <stdio.h>
%}

%option noyywrap
%option yylineno

%%
[a-zA-Z][a-zA-Z0-9]*    printf("word ");
[a-zA-Z0-9\/.-]+        printf("filename ");
\"                      printf("quote ");
\{                      printf("obrace ");
\}                      printf("ebrace ");
;                       printf("semicolon ");
\n                      printf("\n");
[ \t]+                  /* 忽略空白 */
%%