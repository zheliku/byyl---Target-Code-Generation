%{
#include "lex.yy.c"
Node* head=NULL;
#ifndef YYSTYPE
#define YYSTYPE Node*
#endif

#define YYERROR_VERBOSE 1
void yyerror(const char *s);
void m_yyerror(char* msg,int lineno);
char message[100];
%}
%locations
/*1 Tokens*/
%token SEMI COMMA ASSIGNOP RELOP 
%token PLUS MINUS STAR DIV 
%token AND OR DOT NOT TYPE 
%token LP RP LB RB LC RC 
%token STRUCT RETURN IF ELSE WHILE
%token ID INT FLOAT
/*优先级，结合性*/
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%right ASSIGNOP
%left OR
%left AND
%left RELOP
%left PLUS MINUS
%left STAR DIV
%right NOT
%left DOT LP RP LB RB
%%
/*2 High-level Definitions*/
Program : ExtDefList {$$=createNode(Program);insertNode($$,1,$1);head=$$;}
	;
ExtDefList : ExtDef ExtDefList {$$=createNode(ExtDefList);insertNode($$,2,$1,$2);}
	| /* empty */ {$$=createNode(ExtDefList);insertNode($$,1,createNode(None));}
	;
ExtDef : Specifier ExtDecList SEMI {$$=createNode(ExtDef);insertNode($$,3,$1,$2,$3);}
	| Specifier SEMI {$$=createNode(ExtDef);insertNode($$,2,$1,$2);}
	| Specifier FunDec CompSt {$$=createNode(ExtDef);insertNode($$,3,$1,$2,$3);}
	| Specifier error SEMI {$$=createNode(ExtDef);insertNode($$,3,$1,createNode(None),$3);
							m_yyerror("something wrong with ExtDecList before \";\"",@2.last_line);}
	| error SEMI {$$=createNode(ExtDef);insertNode($$,2,createNode(None),$2);
					m_yyerror("something wrong with Specifier before \";\"",@1.last_line);}
	;
ExtDecList : VarDec {$$=createNode(ExtDecList);insertNode($$,1,$1);}
	| VarDec COMMA ExtDecList {$$=createNode(ExtDecList);insertNode($$,3,$1,$2,$3);}
	;
/*3 Specifiers*/
Specifier : TYPE {$$=createNode(Specifier);insertNode($$,1,$1);}
	| StructSpecifier {$$=createNode(Specifier);insertNode($$,1,$1);}
	;
StructSpecifier : STRUCT OptTag LC DefList RC {$$=createNode(StructSpecifier);insertNode($$,5,$1,$2,$3,$4,$5);}
	| STRUCT Tag {$$=createNode(StructSpecifier);insertNode($$,2,$1,$2);}
	;
OptTag : ID {$$=createNode(OptTag);insertNode($$,1,$1);}
	| /* empty */ {$$=createNode(OptTag);insertNode($$,1,createNode(None));}
	;
Tag : ID {$$=createNode(Tag);insertNode($$,1,$1);}
	;
/*4 Declarators*/
VarDec : ID {$$=createNode(VarDec);insertNode($$,1,$1);}
	| VarDec LB INT RB {$$=createNode(VarDec);insertNode($$,4,$1,$2,$3,$4);}
	| VarDec LB error RB {$$=createNode(VarDec);insertNode($$,4,$1,$2,createNode(None),$4);
							m_yyerror("missing a integer between []",@3.last_line);}
	;
FunDec : ID LP VarList RP {$$=createNode(FunDec);insertNode($$,4,$1,$2,$3,$4);}
	| ID LP RP {$$=createNode(FunDec);insertNode($$,3,$1,$2,$3);}
	| ID LP error RP {$$=createNode(FunDec);insertNode($$,4,$1,$2,createNode(None),$4);
						m_yyerror("something wrong with VarList between ()",@3.last_line);}
	| ID error RP {$$=createNode(FunDec);insertNode($$,3,$1,createNode(None),$3);
					m_yyerror("missing \"(\"",@2.last_line);}
	;
VarList : ParamDec COMMA VarList {$$=createNode(VarList);insertNode($$,3,$1,$2,$3);}
	| ParamDec {$$=createNode(VarList);insertNode($$,1,$1);}
	| error COMMA VarList {$$=createNode(VarList);insertNode($$,3,createNode(None),$2,$3);
							m_yyerror("something wrong with ParamDec",@1.last_line);}
	;
ParamDec : Specifier VarDec {$$=createNode(ParamDec);insertNode($$,2,$1,$2);}
	;
/*5 Statements*/
CompSt : LC DefList StmtList RC {$$=createNode(CompSt);insertNode($$,4,$1,$2,$3,$4);}
	| error RC {$$=createNode(CompSt);insertNode($$,2,createNode(None),$2);
				m_yyerror("Missing \"{\"",@1.first_line);}
	;
StmtList : Stmt StmtList {$$=createNode(StmtList);insertNode($$,2,$1,$2);}
	| /* empty */ {$$=createNode(StmtList);insertNode($$,1,createNode(None));}
	;
Stmt : Exp SEMI {$$=createNode(Stmt);insertNode($$,2,$1,$2);}
	| error SEMI {$$=createNode(Stmt);insertNode($$,2,createNode(None),$2);
					m_yyerror("something wrong with expression before \";\"",@1.last_line);}
	| CompSt {$$=createNode(Stmt);insertNode($$,1,$1);}
	| RETURN Exp SEMI {$$=createNode(Stmt);insertNode($$,3,$1,$2,$3);}
	| RETURN error SEMI {$$=createNode(Stmt);insertNode($$,3,$1,createNode(None),$3);
						m_yyerror("something wrong with expression before \";\"",@2.last_line);}
	| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {$$=createNode(Stmt);insertNode($$,5,$1,$2,$3,$4,$5);}
	| IF LP error RP Stmt %prec LOWER_THAN_ELSE{$$=createNode(Stmt);insertNode($$,5,$1,$2,createNode(None),$4,$5);
												m_yyerror("something wrong with expression between ()",@3.last_line);}
	| IF LP Exp RP Stmt ELSE Stmt {$$=createNode(Stmt);insertNode($$,7,$1,$2,$3,$4,$5,$6,$7);}
	| IF LP Exp RP error ELSE Stmt {$$=createNode(Stmt);insertNode($$,7,$1,$2,$3,$4,createNode(None),$6,$7);
									m_yyerror("Missing \";\"",@5.last_line);}
	| IF LP error RP Stmt ELSE Stmt {$$=createNode(Stmt);insertNode($$,7,$1,$2,createNode(None),$4,$5,$6,$7);
									m_yyerror("something wrong with expression between ()",@3.last_line);}
	| WHILE LP Exp RP Stmt {$$=createNode(Stmt);insertNode($$,5,$1,$2,$3,$4,$5);}
	| WHILE LP error RP Stmt {$$=createNode(Stmt);insertNode($$,5,$1,$2,createNode(None),$4,$5);
							m_yyerror("something wrong with expression between ()",@3.last_line);}
	| IF error RP Stmt %prec LOWER_THAN_ELSE {$$=createNode(Stmt);insertNode($$,4,$1,createNode(None),$3,$4);
												m_yyerror("missing \"(\"",@2.last_line);}
	| IF error RP Stmt ELSE Stmt {$$=createNode(Stmt);insertNode($$,6,$1,createNode(None),$3,$4,$5,$6);
									m_yyerror("missing \"(\"",@2.last_line);}
	| WHILE error RP Stmt {$$=createNode(Stmt);insertNode($$,4,$1,createNode(None),$3,$4);
							m_yyerror("missing \"(\"",@2.last_line);}
	;
/*6 Local Definitions*/
DefList : Def DefList {$$=createNode(DefList);insertNode($$,2,$1,$2);}
	| /* empty */ {$$=createNode(DefList);insertNode($$,1,createNode(None));}
	;
Def : Specifier DecList SEMI {$$=createNode(Def);insertNode($$,3,$1,$2,$3);}
	| Specifier error SEMI {$$=createNode(Def);insertNode($$,3,$1,createNode(None),$3);
							m_yyerror("unnecessary \",\"",@2.last_line);}
	;
DecList : Dec {$$=createNode(DecList);insertNode($$,1,$1);}
	| Dec COMMA DecList {$$=createNode(DecList);insertNode($$,3,$1,$2,$3);}
	| error COMMA DecList {$$=createNode(DecList);insertNode($$,3,createNode(None),$2,$3);
							m_yyerror("something wrong with declaration",@1.last_line);}
	;
Dec : VarDec {$$=createNode(Dec);insertNode($$,1,$1);}
	| VarDec ASSIGNOP Exp {$$=createNode(Dec);insertNode($$,3,$1,$2,$3);}
	| error ASSIGNOP Exp {$$=createNode(Dec);insertNode($$,3,createNode(None),$2,$3);
							m_yyerror("missing the variable",@1.last_line);}
	;
/*7 Expressions*/
Exp : Exp ASSIGNOP Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp AND Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp OR Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp RELOP Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp PLUS Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp MINUS Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp STAR Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp DIV Exp {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| LP Exp RP {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| MINUS Exp %prec NOT {$$=createNode(Exp);insertNode($$,2,$1,$2);}
	| NOT Exp {$$=createNode(Exp);insertNode($$,2,$1,$2);}
	| ID LP Args RP {$$=createNode(Exp);insertNode($$,4,$1,$2,$3,$4);}
	| ID LP RP {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| Exp LB Exp RB {$$=createNode(Exp);insertNode($$,4,$1,$2,$3,$4);}
	| Exp DOT ID {$$=createNode(Exp);insertNode($$,3,$1,$2,$3);}
	| ID {$$=createNode(Exp);insertNode($$,1,$1);}
	| INT {$$=createNode(Exp);insertNode($$,1,$1);}
	| FLOAT {$$=createNode(Exp);insertNode($$,1,$1);}
	| error RP {$$=createNode(Exp);insertNode($$,2,createNode(None),$2);
				m_yyerror("missing \"(\"",@1.last_line);}
	| Exp LB error RB {$$=createNode(Exp);insertNode($$,4,$1,$2,createNode(None),$4);
				m_yyerror("missing \"]\"",@3.last_line);}
	| ID error RP {$$=createNode(Exp);insertNode($$,3,$1,createNode(None),$3);
				m_yyerror("missing \"(\"",@2.last_line);}
	;
Args : Exp COMMA Args {$$=createNode(Args);insertNode($$,3,$1,$2,$3);}
	| Exp {$$=createNode(Args);insertNode($$,1,$1);}
	| error COMMA Args {$$=createNode(Args);insertNode($$,3,createNode(None),$2,$3);
						m_yyerror("something wrong with your expression",@1.last_line);}
	;
%%
void  yyerror(const char* msg) {
    //printf("Errir type B at line %d : %s\n", error_line, msg);
    //do nothing
}

void m_yyerror(char* msg,int lineno) {
printf("Error type B at Line %d: %s, maybe %s.\n",lineno,message,msg);
error_occured=true;
}

