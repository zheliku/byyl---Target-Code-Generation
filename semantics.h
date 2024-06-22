#include "tree.h"
#include "symbols.h"

typedef struct Stack {
    VarType type;                                 //当前处理类型：int，float，用户定义类型
    struct TypeDefUnit *structTypeDefUnit;        //当前处理的用户定义类型的定义结构体指针（如果需要）
    int paraNum;                                  //参数数目。
    VarDefUnit *paras[512];                       //各参数定义。不想些链表了。一个结构体、一个函数的变量，参数不超过512个。
    int needNum;                                  //是否需要记录定义的变量。
    int structing;                                //是否正在定义结构体，用于标志当前变量定义是否是结构体内的域定义
    struct Stack *next;
} Stack;

typedef enum idType {
    _IdVal_, _IdFunc_
} idType;

int getErrorOccured();

void dealWith(Node *h);

void dealWithSpecifier(Node *h);

void dealWithStructSpecifier(Node *h);

void dealWithCompSt(Node *h);

void dealWithExtDef(Node *h);

void dealWithVarDec(Node *h);

void dealWithStmt(Node *h);

void dealWithDec(Node *h);

void dealWithExp(VarType *varType, TypeDefUnit **pTypeDefUnit, Node *h);

//根据传入的名称(h->name)和类型，检查id是否存在。不存在则报错返回null，存在则返回定义指针。
void *checkId(Node *h, idType identity);

//检查传入的Node*是否可以有左值
int checkLeft(Node *h);

void push();

void pop();