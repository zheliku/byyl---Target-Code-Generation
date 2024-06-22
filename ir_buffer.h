#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "symbols.h"

#ifndef _IR_BUFFER_H_
#define _IR_BUFFER_H_

//双向循环链表，存储代码在内存中的表示
typedef struct IrNode {
    struct IrNode *prev;        //前一个
    struct IrNode *next;        //后一个
    int argsNum;                //有多少个词。具体见上面注释内的分类。
    char args[6][32];            //每个词都是什么
} IrNode;
#endif

//生成新的label，名称放在提供好的name里面。
void newLabel(char *name);

//生成新的临时变量，名称放在提供好的name里面。
void newTemp(char *name);

//添加一条代码，指明这条代码的词数，然后传入各个词语，各个词语都是char*，即传入多个字符串
void addIrCode(int argsNum, ...);

//将内存中的代码打印到文件中，传入新文件路径，并顺便清理内存中的代码存储
void printCode(char *name);

void irBufferDestroy();
