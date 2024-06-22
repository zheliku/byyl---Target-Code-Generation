%{
#include "tree.h"
#include <stdbool.h>
#include "semantics.h"
#include "translate.h"
#define YYSTYPE Node*
#include "syntax.tab.h"
int yycolumn = 1;
#define YY_USER_ACTION \
	yylloc.first_line = yylloc.last_line = yylineno; \
	yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng;
int get_value(const char* p);
bool error_occured=false;
%}
%option yylineno
blanks [ \r\t]+
letter_ [_A-Za-z]
digit [0-9]
int [1-9]{digit}*|0[0-7]+|0[Xx][0-9A-Fa-f]+|0
float {digit}+\.{digit}+
id {letter_}({letter_}|{digit})*
illOctInt 0[0-9]+
illHexInt 0[Xx][0-9A-Za-z]+
illID {digit}({letter_}|{digit})*
%%
 /*-----------------------------------------|
 |                跳过单行注释                |
 |-----------------------------------------*/
"//" {   
    char c;
    while ((c = input()) != '\n');
}

 /*-----------------------------------------|
 |                跳过多行注释                |
 |-----------------------------------------*/
"/*" {
    char c;
    while ((c = input()) != EOF) {
        if (c == '*') {
            c = input();
            if (c == '/'){
                break;
            }
        }
    }
    if (c == EOF) printf("Error type B at Line %d: LACK OF */.\n", yylineno);
}

{blanks} {}
\n {yycolumn=1;}
; {yylval=createNode(_SEMI);yylval->lineNum=yylineno;return SEMI;}
, {yylval=createNode(_COMMA);yylval->lineNum=yylineno;return COMMA;}
= {yylval=createNode(_ASSIGNOP);yylval->lineNum=yylineno;return ASSIGNOP;}
>|<|>=|<=|==|!= {yylval=createNode(_RELOP);yylval->lineNum=yylineno;strcpy(yylval->name,yytext);return RELOP;}
\+ {yylval=createNode(_PLUS);yylval->lineNum=yylineno;return PLUS;}
\- {yylval=createNode(_MINUS);yylval->lineNum=yylineno;return MINUS;}
\* {yylval=createNode(_STAR);yylval->lineNum=yylineno;return STAR;}
\/ {yylval=createNode(_DIV);yylval->lineNum=yylineno;return DIV;}
&& {yylval=createNode(_AND);yylval->lineNum=yylineno;return AND;}
\|\| {yylval=createNode(_OR);yylval->lineNum=yylineno;return OR;}
\. {yylval=createNode(_DOT);yylval->lineNum=yylineno;return DOT;}
! {yylval=createNode(_NOT);yylval->lineNum=yylineno;return NOT;}
int|float {yylval=createNode(_TYPE);yylval->lineNum=yylineno;strcpy(yylval->name,yytext);return TYPE;}
\( {yylval=createNode(_LP);yylval->lineNum=yylineno;return LP;}
\) {yylval=createNode(_RP);yylval->lineNum=yylineno;return RP;}
\[ {yylval=createNode(_LB);yylval->lineNum=yylineno;return LB;}
\] {yylval=createNode(_RB);yylval->lineNum=yylineno;return RB;}
\{ {yylval=createNode(_LC);yylval->lineNum=yylineno;return LC;}
\} {yylval=createNode(_RC);yylval->lineNum=yylineno;return RC;}
struct {yylval=createNode(_STRUCT);yylval->lineNum=yylineno;return STRUCT;}
return {yylval=createNode(_RETURN);yylval->lineNum=yylineno;return RETURN;}
if {yylval=createNode(_IF);yylval->lineNum=yylineno;return IF;}
else {yylval=createNode(_ELSE);yylval->lineNum=yylineno;return ELSE;}
while {yylval=createNode(_WHILE);yylval->lineNum=yylineno;return WHILE;}
{int} {yylval=createNode(_INT);yylval->lineNum=yylineno;yylval->intValue=get_value(yytext);return INT;}
{float} {yylval=createNode(_FLOAT);yylval->lineNum=yylineno;yylval->floatValue=strtof(yytext,NULL);return FLOAT;}
{id} {yylval=createNode(_ID);yylval->lineNum=yylineno;strcpy(yylval->name,yytext);return ID;}
{illHexInt} {printf("Error type A at Line %d: Illegal hexadecimal number \"%s\"\n",yylineno,yytext);
			error_occured=true;yylval=createNode(None);return INT;}
{illOctInt} {printf("Error type A at Line %d: Illegal octal number \"%s\"\n",yylineno,yytext);
			error_occured=true;yylval=createNode(None);return INT;}
{illID} {printf("Error type A at Line %d: Illegal identifier name \"%s\"\n",yylineno,yytext);
		error_occured=true;yylval=createNode(None);return ID;}
. {printf("Error type A at Line %d: Mysterious character \"%s\"\n",yylineno,yytext);error_occured=true;}
%%


int get_value(const char* p)
{
	long value;
	if(p[0]!='0')
		value=strtol(p,NULL,10);
	else if(p[1]=='x'||p[1]=='X')
		value=strtol(p,NULL,16);
	else value=strtol(p,NULL,8);
	return (int)value;
}