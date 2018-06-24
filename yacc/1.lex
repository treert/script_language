%{
#include <stdio.h>

// #define YY_MAIN 1

int count = 0;
%}

%option noyywrap

D   [0-9]
Word   [0-9a-zA-Z.\\\/-]
PAY (PAY|pay)

%%

    // https?:{Word}{PAY}{Word} {printf("%s\n",yytext);}

(https?|ftp|file):\/\/[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|] {printf("%s\n",yytext);count++;}

.|\n    {};
%%

void main(int argc, char** argv){
    ++argv, --argc;  /* skip over program name */
    if(argc > 0)
        yyin = fopen( argv[0], "rb" );
    else
        yyin = stdin;
    yylex();
    printf("char num %d\n",count);
}