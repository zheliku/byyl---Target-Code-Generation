#include <stdlib.h>
#include <string.h>
#include "tree.h"

#ifndef _SYMBOLS_H_
#define _SYMBOLS_H_

//链表头部插入宏。head为头指针，p为要插入元素的指针。
#define insert_head(head, p) \
    do \
    { \
        (p)->next = (head); \
        (head) = (p); \
    }while(0)

struct TypeDefUnit;
struct VarDefUnit;

//变量的类型：int，float，用户自定义类型（数组，结构体）
typedef enum VarType {
    _INT_, _FLOAT_, _ARR_STRUCT_
} VarType;
//用户自定义类型：结构体，数组
typedef enum ArrayStructType {
    _STRUCT_, _ARRAY_
} ArrayStructType;

//数组定义的单元，数组定义用链表表示。
typedef struct ArrayDefList {
    int dimension;                    //该层的维度。从0层开始。即一级（如int arrayDefList[]）是0层。
    int number;                        //该层有几个元素
    int elemSize;                //每个元素多大。（目前认为，struct的大小和int一样，即4字节）
    VarType elemType;                    //基本元素是什么：int,float,struct
    struct TypeDefUnit *structTypeDefUnit;        //struct定义指针，如果需要
    struct ArrayDefList *next;    //降一维度指针
} ArrayDefList;
//结构体定义单元。该结构体有几个域，每个域的定义
typedef struct StructDefList {
    int defNum;            //该结构体有几个域
    struct VarDefUnit **defList;    //每个域的定义
} StructDefList;

//变量定义单元。有函数参数，用户变量，结构体内变量。
typedef struct VarDefUnit {
    char name[MAX_LEN_OF_NAME];        //名称
    int usable;                //是否是真的变量，即是否可以直接使用，因为结构体中的域不能直接使用
    VarType varType;                    //类型：int，float，用户定义类型
    struct TypeDefUnit *structTypeDefUnit;        //用户定义类型的定义结构体指针（如果需要）
    struct VarDefUnit *next;                //下一个单元地址
} VarDefUnit;
//类型定义单元。
typedef struct TypeDefUnit {
    char name[MAX_LEN_OF_NAME];    //名称。数组没有名称
    ArrayStructType arrayStructType;                //类型：结构体，数组
    union {
        ArrayDefList *arrayDefList;
        StructDefList *structDefList;
    } def;                        //类型具体定义。a为数组定义地址，s为结构体定义地址
    struct TypeDefUnit *next;        //下一个单元地址
} TypeDefUnit;
//函数定义单元。
typedef struct FunDefUnit {
    char name[MAX_LEN_OF_NAME];    //名称
    int parameterNum;        //参数个数
    VarType *varType;            //参数类型标志
    TypeDefUnit **parameters;            //参数定义列表(如果某参数需要)
    VarType returnType;
    struct VarDefUnit **parameterList;
    TypeDefUnit *returnTypeDefUnit;        //返回值类型定义
    struct FunDefUnit *next;        //下一个单元地址
} FunDefUnit;
//为了作用域而实现的变量定义表栈。利用头部插入，取头部，头部删除等实现。
typedef struct ValueStack {
    VarDefUnit *values;                //变量定义表
    struct ValueStack *next;    //下一个
} ValueStack;
#endif

//栈操作函数。push和pop，查看某一个变量能否在栈顶定义（true表示栈顶没有这个变量定义，false表示栈顶已有这个变量定义）。
void valueStackPush();

void valueStackPop();

int valueStackCheck(const char *name);

//下面初始化函数和整个符号表的析构函数。
void initSymbolTable();

void destroySymbolTable();

//下面是3个构造函数，动态分配出新的表项，返回其地址。
TypeDefUnit *newType(const char *name);

FunDefUnit *newFunction(const char *name);

VarDefUnit *newValue(const char *name);

//辅助函数，操作数组定义时候使用。分别为创建一个基类型层，以及在已经有基类型层基础上拓展一个维度。
void arrayGenerateBasicDimension(TypeDefUnit *t, int number, VarType kind, TypeDefUnit *val_type);

void arrayExpandDimension(TypeDefUnit *t, int number);

//辅助函数，判断两个类型是否相等。
int typeEqual(TypeDefUnit *p, TypeDefUnit *q);

//辅助函数。获得下一个别名,别名不会重复，一般而言不会与用户定义变量重名。用于匿名struct等场景使用。别名放在dest中。
void getAName(char *name);

//下面是3个查询函数。查询成功则返回对应定义指针，失败则返回NULL。
TypeDefUnit *getType(const char *name);

FunDefUnit *getFunction(const char *name);

VarDefUnit *getValue(const char *name);

//下面是3个添加函数。在已经构造好表项后，将其添加进符号表中对应的位置。返回刚刚被添加进去的表项。
TypeDefUnit *addTypeDeclaration(TypeDefUnit *r);

FunDefUnit *addFunctionDeclaration(FunDefUnit *r);

VarDefUnit *addValueDeclaration(VarDefUnit *r);

//得到结构体的大小；得到一个域(一定在该结构体中存在)在结构体内从头部开始的偏移量。
int structGetSize(TypeDefUnit *s);

int structGetOffset(TypeDefUnit *s, char *field);

//int to string
void myItoa(unsigned long val, char *buf, unsigned radix);