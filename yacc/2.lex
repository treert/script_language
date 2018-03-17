%{
#include <stdio.h>
#define YY_MAIN 1
%}

%option noyywrap
%option yylineno

%%
[0-9]+	printf("number!");
[_a-zA-Z][_a-zA-Z0-9]*	printf("word!");
%%