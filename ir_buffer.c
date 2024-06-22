#include "ir_buffer.h"
#include "dst_code.h"


//label,temp的计数器，用于生成新的label，temp
static int tempCount = 0, labelCount = 0;

//代码buff链表的头指针。
static IrNode *codeHead = NULL;

//释放中间代码buffer
void irBufferDestroy();

//向文件中打印一行代码
void printOneLine(FILE *fp, IrNode *p);


//生成新的label，名称放在提供好的name里面。
void newLabel(char *name) {
	strcpy(name, "label");
	char temp[20];
	myItoa(labelCount, temp, 10);
	labelCount++;
	strcat(name, temp);
}

//生成新的临时变量，名称放在提供好的name里面。
void newTemp(char *name) {
	strcpy(name, "_t");
	char temp[20];
	myItoa(tempCount, temp, 10);
	tempCount++;
	strcat(name, temp);
}

//添加一条代码，指明这条代码的词数，然后传入各个词语，各个词语都是char*，即传入多个字符串
void addIrCode(int argsNum, ...) {
	IrNode *p = (IrNode *) malloc(sizeof(IrNode));
	p->argsNum = argsNum;
	va_list ap;
	va_start(ap, argsNum);
	for (int i = 0; i < argsNum; i++)
		strcpy(p->args[i], va_arg(ap, char*));
	if (codeHead == NULL) {
		p->prev = p;
		p->next = p;
		codeHead = p;
	}
	else {
		p->next = codeHead;
		p->prev = codeHead->prev;
		codeHead->prev->next = p;
		codeHead->prev = p;
	}
}

//将内存中的代码打印到文件中，传入新文件路径
void printCode(char *name) {
	if (codeHead == NULL)return;
	char irName[32], dstName[32];
	sprintf(irName, "ir_%s", name);
	sprintf(dstName, "dst_%s", name);
	FILE *fp_ir = fopen(irName, "w");
	FILE *fp_dst = fopen(dstName, "w");
	assert(fp_ir != NULL);
	IrNode *p = codeHead;
	do {
		printOneLine(fp_ir, p);
		p = p->next;
	} while (p != codeHead);
	genDstCode(codeHead, fp_dst);
	// ir_buffer_destroy();
	fclose(fp_ir);
	fclose(fp_dst);
}

//释放中间代码buffer
void irBufferDestroy() {
	if (codeHead == NULL)return;
	IrNode *p = codeHead->next, *q;
	free(codeHead);
	while (p != codeHead) {
		q = p;
		p = p->next;
		free(q);
	}
}

//向文件中打印一行代码
void printOneLine(FILE *fp, IrNode *p) {
	if (p->argsNum != 3 && p->args[0][0] == '*') {
		char temp[32];
		newTemp(temp);
		fprintf(fp, "%s", temp);
		for (int i = 1; i < p->argsNum; i++)
			fprintf(fp, " %s", p->args[i]);
		fprintf(fp, "\n");
		fprintf(fp, "%s := %s\n", p->args[0], temp);
	}
	else if (p->argsNum == 2 && p->args[1][0] == '*' && strcmp(p->args[0], "READ") == 0) {
		char temp[32];
		newTemp(temp);
		fprintf(fp, "READ %s\n", &temp[1]);
		fprintf(fp, "%s := %s\n", p->args[1], &temp[1]);
	}
	else {
		fprintf(fp, "%s", p->args[0]);
		for (int i = 1; i < p->argsNum; i++)
			fprintf(fp, " %s", p->args[i]);
		fprintf(fp, "\n");
	}
}
