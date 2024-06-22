#include <stdio.h>
#include "tree.h"
#include "semantics.h"
#include "symbols.h"
#include "translate.h"
#include "log.h"

extern void yyrestart(FILE *);

extern int yyparse(void);

extern Node *head;
extern int error_occured;

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("请输入一个源文件，一个目标文件\n");
		return 1;
	}
	FILE *f = fopen(argv[1], "r");
	if (f == NULL) {
		printf("无法打开文件 %s\n", argv[1]);
		return 1;
	}
	yyrestart(f);
	if (yyparse() == 0) {
		FILE *treeFile = fopen("treeFile.txt", "w");
		printTree(head, treeFile);
		SemanticError = initLog();
		initSymbolTable();
		dealWith(head);
		outputLog(SemanticError, stdout);
		translate(head);
		printCode(argv[2]);
		destroySymbolTable();
		irBufferDestroy();
		fclose(treeFile);
	}
	destroyTree(head);
	fclose(f);
	return 0;
}