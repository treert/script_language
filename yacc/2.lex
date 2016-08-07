%{
#include <stdio.h>
%}
%%
[0-9]+	printf("number!");
[_a-zA-Z][_a-zA-Z0-9]*	printf("word!");
%%