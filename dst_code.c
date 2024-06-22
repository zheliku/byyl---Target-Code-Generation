#include "dst_code.h"

typedef struct Variable {
	char name[32];
	int offset;
} Variable;

static IrNode *head;
FILE *fp;
static int argCount = 0;
static int size = 0;
static int count = 0;
// 变量表
static Variable table[1024];

/**
 * 常数返回-1
 * *&开头，不看开头
 * 在table里找名字，返回offset
 * @param name
 * @return
 */
int getOffset(char *name) {
	if (name[0] == '#')return -1;
	if (name[0] == '*' || name[0] == '&')name++;
	for (int i = 0; i < count; i++) {
		if (strcmp(name, table[i].name) == 0)
			return table[i].offset;
	}
	return -1;
}

/**
 * 往table里加一个variable
 * @param name
 * @param sz
 */
void addVar(char *name, int sz) {
	if (name[0] == '#')return;
	if (name[0] == '*' || name[0] == '&')name++;
	if (getOffset(name) != -1)return;
	table[count].offset = size;
	size += sz;
	strcpy(table[count].name, name);
	count++;
}


void prepareReg(char *name, int num) {
	char temp[8];
	sprintf(temp, "$t%d", num);
	if (name[0] == '*') {
		// 第一步：把相对栈顶偏移loc的位置的值赋给temp
		fprintf(fp, "  lw %s, %d($sp)\n", temp, getOffset(name));
		// 第二步：将temp的值与$sp地址相加结果存入temp
		fprintf(fp, "  add %s, %s, $sp\n", temp, temp);
		// 第三步：使用temp中的最终地址找到实际要找的值
		fprintf(fp, "  lw %s, 0(%s)\n", temp, temp);
	}
	else if (name[0] == '&') {
		// 将name的地址存到t寄存器中
		fprintf(fp, "  li %s, %d\n", temp, getOffset(name));
	}
	else if (name[0] == '#') {
		// 存常数
		fprintf(fp, "  li %s, %s\n", temp, &name[1]);
	}
	else {
		// 普通变量从栈里拿
		fprintf(fp, "  lw %s, %d($sp)\n", temp, getOffset(name));
	}
}

//翻译一个函数，两参数左闭右开。
void genOneFunction(IrNode *begin, IrNode *end);

void genDstCode(IrNode *h, FILE *f) {
	head = h;
	fp = f;
	fprintf(fp, "%s\n", ".data");
	fprintf(fp, "%s\n", "_prompt: .asciiz \"Enter an integer:\"");
	fprintf(fp, "%s\n", "_ret: .asciiz \"\\n\"");
	fprintf(fp, "%s\n", ".globl main");
	fprintf(fp, "%s\n", ".text");
	fprintf(fp, "%s\n", "read:");
	fprintf(fp, "  %s\n", "li $v0, 4");
	fprintf(fp, "  %s\n", "la $a0, _prompt");
	fprintf(fp, "  %s\n", "syscall");
	fprintf(fp, "  %s\n", "li $v0, 5");
	fprintf(fp, "  %s\n", "syscall");
	fprintf(fp, "  %s\n", "jr $ra");
	fprintf(fp, "\n");
	fprintf(fp, "%s\n", "write:");
	fprintf(fp, "  %s\n", "li $v0, 1");
	fprintf(fp, "  %s\n", "syscall");
	fprintf(fp, "  %s\n", "li $v0, 4");
	fprintf(fp, "  %s\n", "la $a0, _ret");
	fprintf(fp, "  %s\n", "syscall");
	fprintf(fp, "  %s\n", "move $v0, $0");
	fprintf(fp, "  %s\n", "jr $ra");

	IrNode *p = head, *q;
	do {
		// 定位函数
		q = p->next;
		while (1) {
			if (strcmp(q->args[0], "FUNCTION") == 0 || q == head) break;
			q = q->next;
		}
		// 分析函数
		genOneFunction(p, q);
		p = q;
	} while (p != head);
}

void genOneFunction(IrNode *begin, IrNode *end) {
	count = 0;
	size = 0;
	argCount = 0;
	// 输出函数
	fprintf(fp, "\n%s:\n", begin->args[1]);

	// 第一次处理，产生变量表
	IrNode *p = begin->next;
	while (p != end) {
		switch (p->argsNum) {
			case 2: {
				if (strcmp("GOTO", p->args[0]) != 0)
					addVar(p->args[1], 4);
				break;
			}
			case 3: {
				if (strcmp(p->args[1], ":=") == 0) {
					addVar(p->args[0], 4);
					addVar(p->args[2], 4);
				}
				else if (strcmp(p->args[0], "DEC") == 0) {
					int a = strtol(p->args[2], NULL, 10);
					addVar(p->args[1], a);
				}
				break;
			}
			case 4: {
				addVar(p->args[0], 4);
				break;
			}
			case 5: {
				addVar(p->args[0], 4);
				addVar(p->args[2], 4);
				addVar(p->args[4], 4);
				break;
			}
			case 6: {
				addVar(p->args[1], 4);
				addVar(p->args[3], 4);
				break;
			}
			default:
				break;
		}
		p = p->next;
	}
	// 更改栈顶指针位置
	fprintf(fp, "  addi $sp, $sp, -%d\n", size);

	// 第二次处理，生成代码
	p = begin->next;
	while (p != end) {
		switch (p->argsNum) {
			case 2: {
				// GOTO x
				if (strcmp(p->args[0], "GOTO") == 0) {
					fprintf(fp, "  j %s\n", p->args[1]);
				}
				// RETURN x
				else if (strcmp(p->args[0], "RETURN") == 0) {
					prepareReg(p->args[1], 0);
					fprintf(fp, "  move $v0, $t0\n");
					fprintf(fp, "  addi $sp, $sp, %d\n", size);
					fprintf(fp, "  jr $ra\n");
				}
				// ARG x
				else if (strcmp(p->args[0], "ARG") == 0) {
					prepareReg(p->args[1], 0);
					fprintf(fp, "  move $a%d, $t0\n", argCount);
					argCount++;
				}
				// PARAM x
				else if (strcmp(p->args[0], "PARAM") == 0) {
					int para_count = 0;
					IrNode *q = p;
					while (strcmp(q->next->args[0], "PARAM") == 0) {
						q = q->next;
						para_count++;
					}
					fprintf(fp, "  sw $a%d, %d($sp)\n", para_count, getOffset(p->args[1]));
				}
				// READ x
				else if (strcmp(p->args[0], "READ") == 0) {
					// 保存上下文环境
					fprintf(fp, "  addi $sp, $sp, -4\n");
					fprintf(fp, "  sw $ra, 0($sp)\n");
					fprintf(fp, "  jal read\n");
					fprintf(fp, "  lw $ra, 0($sp)\n");
					fprintf(fp, "  addi $sp, $sp, 4\n");
					if (p->args[1][0] == '*') {
						// 类似 prepareReg
						fprintf(fp, "  lw $t0, %d($sp)\n", getOffset(p->args[1]));
						fprintf(fp, "  add $t0, $t0, $sp\n");
						fprintf(fp, "  sw $v0, 0($t0)\n");
					}
					else
						fprintf(fp, "  sw $v0, %d($sp)\n", getOffset(p->args[1]));
				}
				// WRITE x
				else if (strcmp(p->args[0], "WRITE") == 0) {
					prepareReg(p->args[1], 0);
					// 获取参数
					fprintf(fp, "  move $a0, $t0\n");
					// 保存上下文环境
					fprintf(fp, "  addi $sp, $sp, -4\n");
					fprintf(fp, "  sw $ra, 0($sp)\n");
					fprintf(fp, "  jal write\n");
					fprintf(fp, "  lw $ra, 0($sp)\n");
					fprintf(fp, "  addi $sp, $sp, 4\n");
				}
				break;
			}
			case 3: {
				// x := y
				if (strcmp(p->args[1], ":=") == 0) {
					prepareReg(p->args[2], 0);
					if (p->args[0][0] == '*') {
						// 同 prepareReg
						fprintf(fp, "  lw $t1, %d($sp)\n", getOffset(p->args[0]));
						fprintf(fp, "  add $t1, $t1, $sp\n");
						fprintf(fp, "  sw $t0, 0($t1)\n");
					}
					else {
						fprintf(fp, "  sw $t0, %d($sp)\n", getOffset(p->args[0]));
					}
				}
				// DEC x [size]
				else if (strcmp(p->args[0], "DEC") != 0) {
					fprintf(fp, "%s:\n", p->args[1]);
				}
				break;
			}
			case 4: {
				// x := CALL f
				argCount = 0;
				fprintf(fp, "  addi $sp, $sp, -4\n");
				fprintf(fp, "  sw $ra, 0($sp)\n");
				fprintf(fp, "  jal %s\n", p->args[3]);
				fprintf(fp, "  lw $ra, 0($sp)\n");
				fprintf(fp, "  addi $sp, $sp, 4\n");
				if (p->args[0][0] == '*') {
					fprintf(fp, "  lw $t0, %d($sp)\n", getOffset(p->args[0]));
					fprintf(fp, "  add $t0, $t0 ,$sp\n");
					fprintf(fp, "  sw $v0, 0($t0)\n");
				}
				else
					fprintf(fp, "  sw $v0, %d($sp)\n", getOffset(p->args[0]));
				break;
			}
			case 5: {
				prepareReg(p->args[2], 0);
				prepareReg(p->args[4], 1);
				// x := y + z    x := y - z    x := y * z    x := y / z
				switch (p->args[3][0]) {
					case '+': {
						fprintf(fp, "  add $t0, $t0, $t1\n");
						break;
					}
					case '-': {
						fprintf(fp, "  sub $t0, $t0, $t1\n");
						break;
					}
					case '*': {
						fprintf(fp, "  mul $t0, $t0, $t1\n");
						break;
					}
					case '/': {
						fprintf(fp, "  div $t0, $t1\n");
						fprintf(fp, "  mflo $t0\n");
						break;
					}
					default:;
				}
				if (p->args[0][0] == '*') {
					fprintf(fp, "  lw $t1, %d($sp)\n", getOffset(p->args[0]));
					fprintf(fp, "  add $t1, $t1, $sp\n");
					fprintf(fp, "  sw $t0, 0($t1)\n");
				}
				else
					fprintf(fp, "  sw $t0, %d($sp)\n", getOffset(p->args[0]));
				break;
			}
			case 6: {
				// if x [relop] y GOTO z
				char temp[4];
				if (strcmp(p->args[2], "==") == 0)
					strcpy(temp, "beq");
				else if (strcmp(p->args[2], "!=") == 0)
					strcpy(temp, "bne");
				else if (strcmp(p->args[2], ">") == 0)
					strcpy(temp, "bgt");
				else if (strcmp(p->args[2], "<") == 0)
					strcpy(temp, "blt");
				else if (strcmp(p->args[2], ">=") == 0)
					strcpy(temp, "bge");
				else if (strcmp(p->args[2], "<=") == 0)
					strcpy(temp, "ble");
				prepareReg(p->args[1], 0);
				prepareReg(p->args[3], 1);
				fprintf(fp, "  %s $t0, $t1, %s\n", temp, p->args[5]);
				break;
			}
			default:
				break;
		}
		p = p->next;
	}
}