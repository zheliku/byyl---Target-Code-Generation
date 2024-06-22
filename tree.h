#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef _TREE_H_
#define _TREE_H_
#define MAX_LEN_OF_NAME 32+1
#define MAX_COUNT_OF_CHILD 7
typedef enum Types {
    /*0 Epsilon*/
    None,
    /*1 Tokens*/
    _SEMI, _COMMA, _ASSIGNOP, _RELOP,
    _PLUS, _MINUS, _STAR, _DIV,
    _AND, _OR, _DOT, _NOT, _TYPE,
    _LP, _RP, _LB, _RB, _LC, _RC,
    _STRUCT, _RETURN, _IF, _ELSE, _WHILE,
    _ID, _INT, _FLOAT,
    /*2 High-level Definitions*/
    Program, ExtDefList, ExtDef, ExtDecList,
    /*3 Specifiers*/
    Specifier, StructSpecifier, OptTag, Tag,
    /*4 Declarators*/
    VarDec, FunDec, VarList, ParamDec,
    /*5 Statements*/
    CompSt, StmtList, Stmt,
    /*6 Local Definitions*/
    DefList, Def, DecList, Dec,
    /*7 Expressions*/
    Exp, Args
} Types;

typedef struct Node {
    Types type;                   //类型
    char name[MAX_LEN_OF_NAME];   //ID的名称
    int lineNum;                  //行号
    int intValue;                 //整形数值
    float floatValue;             //浮点形数值
    struct Node *father;          //父节点
    int childNum;                 //字节点的数量
    struct Node *child[MAX_COUNT_OF_CHILD];
} Node;
#endif

Node *createNode(Types type);

void destroyTree(Node *head);

void insertNode(Node *father, int n, ...);

void printTree(Node *head, FILE *f);

const char *getTypeName(Types type);