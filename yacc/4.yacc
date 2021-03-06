%{
#include <stdio.h>
#include <string.h>
extern int yylineno;
void yyerror(const char *str){
    fprintf(stderr,"error:%s line:%d\n",str,yylineno);
}

main()
{
    yyparse();
}

%}

%token NUMBER TOKHEAT STATE TOKTARGET TOKTEMPERATURE

%%
commands: /* empty */
                | commands command
                ;

command:
        heat_switch
        |
        target_set
        ;

heat_switch:
        TOKHEAT STATE
        {
        	if($2)
        		printf("\tHeat turned on\n");
        	else
        		printf("\tHeat turned off\n");
        }
        ;
target_set:
        TOKTARGET TOKTEMPERATURE NUMBER
        {
            printf("\tTemperature set %d\n",$3);
        } 
        ;