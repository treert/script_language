%{
#include <stdio.h>

#define YY_MAIN 1
%}

%option noyywrap

%%
stop	printf("stop!");
start	printf("start!");
%%