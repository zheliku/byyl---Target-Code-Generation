#ifndef __LOG_H_
#define __LOG_H_

#include <stdbool.h>
#include <stdio.h>
#include "tree.h"

#define MESSAGE_LENGTH 256

typedef struct Info {
	char *content;
	struct Info *next;
} Info;

typedef struct Log {
	Info *head;
	Info *tail;
} Log;

extern Log *SemanticError;

Log *initLog();

bool addLogInfo(Log *log, char *content);

bool outputLog(Log *log, FILE *file);

bool reportError(Log *log, char *message);

#endif
