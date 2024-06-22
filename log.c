#include "log.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "tree.h"

Log *SemanticError = NULL;

Log *initLog() {
	Log *log = (Log *) malloc(sizeof(Log));
	if (log == NULL) return NULL;
	log->head = NULL;
	log->tail = NULL;
	return log;
}

bool addLogInfo(Log *log, char *content) {
	if (log == NULL) {
		printf("The Log is NULL.\n");
		return false;
	}
	else {
		if (log->head == NULL && log->tail == NULL) {
			Info *i = (Info *) malloc(sizeof(Info));
			i->content = (char *) malloc(strlen(content) + 1);
			strcpy(i->content, content);
			i->next = NULL;
			log->head = i;
			log->tail = i;
		}
		else if (log->head != NULL && log->tail != NULL) {
			Info *i = (Info *) malloc(sizeof(Info));
			i->content = (char *) malloc(strlen(content) + 1);
			strcpy(i->content, content);
			i->next = NULL;
			log->tail->next = i;
			log->tail = i;
		}
		else {
			printf("This Log is broken.\n");
			return false;
		}
	}
	return true;
}

bool outputLog(Log *log, FILE *file) {
	// if (log == NULL) {
	// 	printf("The Log is NULL.\n");
	// 	return false;
	// }
	// Info *p = log->head;
	// while (p != NULL) {
	// 	fprintf(file, "%s", p->content);
	// 	p = p->next;
	// }
	return true;
}

bool reportError(Log *log, char *message) {
	return addLogInfo(log, message);
}
