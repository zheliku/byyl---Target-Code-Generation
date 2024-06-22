#include "tree.h"

void add(Node *father, int loc, Node *child);

static int depth = -1;
static const char *const types_name_table[] = {
        /*0 Epsilon*/
        "None",
        /*1 Tokens*/
        "SEMI", "COMMA", "ASSIGNOP", "RELOP",
        "PLUS", "MINUS", "STAR", "DIV",
        "AND", "OR", "DOT", "NOT", "TYPE",
        "LP", "RP", "LB", "RB", "LC", "RC",
        "STRUCT", "RETURN", "IF", "ELSE", "WHILE",
        "ID", "INT", "FLOAT",
        /*2 High-level Definitions*/
        "Program", "ExtDefList", "ExtDef", "ExtDecList",
        /*3 Specifiers*/
        "Specifier", "StructSpecifier", "OptTag", "Tag",
        /*4 Declarators*/
        "VarDec", "FunDec", "VarList", "ParamDec",
        /*5 Statements*/
        "CompSt", "StmtList", "Stmt",
        /*6 Local Definitions*/
        "DefList", "Def", "DecList", "Dec",
        /*7 Expressions*/
        "Exp", "Args"
};

const char *getTypeName(Types type) {
    return types_name_table[type];
}

Node *createNode(Types type) {
    Node *p = (Node *) malloc(sizeof(Node));
    if (p == NULL) {
        printf("Memory allocation error!\n");
        exit(0);
    }
    p->intValue = 0;
    p->floatValue = 0.0;
    p->type = type;
    p->name[0] = '\0';
    p->lineNum = -1;
    p->father = NULL;
    p->childNum = 0;
    for (int i = 0; i < MAX_COUNT_OF_CHILD; i++)
        p->child[i] = NULL;
    return p;
}

void add(Node *father, int loc, Node *child) {
    father->child[loc] = child;
    child->father = father;
    if (loc == 0)
        father->lineNum = child->lineNum;
}

void destroyTree(Node *head) {
    if (head == NULL)
        return;
    for (int i = 0; i < head->childNum; i++)
        destroyTree(head->child[i]);
    free(head);
    head = NULL;
}

void insertNode(Node *father, int n, ...) {
    va_list ap;
    va_start(ap, n);
    father->childNum = n;
    for (int i = 0; i < n; i++)
        add(father, i, va_arg(ap, Node*));
}

void printTree(Node *head, FILE *f) {
    if (head == NULL) return;
    if (head->lineNum == -1) return;
    depth++;
    for (int i = 0; i < depth; i++)
        fprintf(f, "  ");
    fprintf(f, "%s", getTypeName(head->type));
    if (head->type > _FLOAT)
        fprintf(f, " (%d)", head->lineNum);
    else if (head->type == _ID || head->type == _TYPE)
        fprintf(f, ": %structDefList", head->name);
    else if (head->type == _INT)
        fprintf(f, ": %d", head->intValue);
    else if (head->type == _FLOAT)
        fprintf(f, ": %f", head->floatValue);
    fprintf(f, "\n");
    for (int i = 0; i < head->childNum; i++)
        printTree(head->child[i], f);
    depth--;
}