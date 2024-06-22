#include "translate.h"

//当前的struct定义处，用于嵌套定义struct的访问。
static TypeDefUnit *currentStruct = NULL;
//当前数组单元大小。
static int currentSize = 4;
//当前函数参数表，用来确定某一个变量是结构体的引用还是结构体本身。
static FunDefUnit *currentFunc = NULL;

const int MAX_LENGTH = 32;

/*
 * 翻译exp，其中的option表示place的主动、被动。
 * 0：表示 exp 没有用过，是一个新遇见的 exp
 * 1：表示 exp 已经用过
 * 如果place==NULL,则本层不要生成变量返回该exp的结果。
 */
void translateExp(Node *h, char *place, int option);

//翻译COND，由上层提供跳转label
void translateCond(Node *h, char *label_true, char *label_false);

void translateFunDec(Node *h);

void translateDec(Node *h);

void translateStmt(Node *h);
//得到这个exp的地址。
void getLocation(Node *h, char *place);

//翻译参数列表。需要当前函数定义中的参数表，当前是第几个参数。
void translateArgs(Node *h, VarDefUnit **args, int count);

void translate(Node *h) {
    if (h == NULL) return;
    switch (h->type) {
        case Specifier:
            return;
        case FunDec: {
            translateFunDec(h);
            break;
        }
        case Dec: {
            translateDec(h);
            break;
        }
        case Stmt: {
            translateStmt(h);
            break;
        }
        default: {
            for (int i = 0; i < h->childNum; i++)
                translate(h->child[i]);
            break;
        }
    }
}

void translateStmt(Node *h) {
    switch (h->child[0]->type) {
        case Exp: {
            translateExp(h->child[0], NULL, 0);
            break;
        }
        case CompSt: {
            translate(h->child[0]);
            break;
        }
        case _RETURN: {
            char temp[MAX_LENGTH];
            translateExp(h->child[1], temp, 0);
            addIrCode(2, "RETURN", temp);
            break;
        }
        case _IF: {
            char label1[MAX_LENGTH], label2[MAX_LENGTH], label3[MAX_LENGTH];
            newLabel(label1);
            newLabel(label2);
            if (h->childNum == 7) {
                newLabel(label3);
            }
            translateCond(h->child[2], label1, label2);
            addIrCode(3, "LABEL", label1, ":");
            translate(h->child[4]);
            if (h->childNum == 7)
                addIrCode(2, "GOTO", label3);
            addIrCode(3, "LABEL", label2, ":");
            if (h->childNum == 7) {
                translate(h->child[6]);
                addIrCode(3, "LABEL", label3, ":");
            }
            break;
        }
        case _WHILE: {
            char label1[MAX_LENGTH], label2[MAX_LENGTH], label3[MAX_LENGTH];
            newLabel(label1);
            newLabel(label2);
            newLabel(label3);
            addIrCode(3, "LABEL", label1, ":");
            translateCond(h->child[2], label2, label3);
            addIrCode(3, "LABEL", label2, ":");
            translate(h->child[4]);
            addIrCode(2, "GOTO", label1);
            addIrCode(3, "LABEL", label3, ":");
            break;
        }
    }
}


void translateDec(Node *h) {
    Node *p = h->child[0]->child[0];
    if (p->type != _ID) {
        p = p->child[0];
        while (p->type != _ID)
            p = p->child[0];
    }
    VarDefUnit *temp = getValue(p->name);
    if (temp->varType == _ARR_STRUCT_) {
        char length[8];
        myItoa(structGetSize(temp->structTypeDefUnit), length, 10);
        addIrCode(3, "DEC", temp->name, length);
    }
    if (h->childNum == 3) {
        translateExp(h->child[2], temp->name, 1);
    }
}


void translateFunDec(Node *h) {
    FunDefUnit *temp = getFunction(h->child[0]->name);
    addIrCode(3, "FUNCTION", h->child[0]->name, ":");
    for (int i = 0; i < temp->parameterNum; i++)
        addIrCode(2, "PARAM", temp->parameterList[i]->name);
    currentFunc = temp;
}

void translateExp(Node *h, char *place, int option) {
    if (h->child[0]->type == _LP) {
        return translateExp(h->child[1], place, option);
    } else if (h->childNum == 3 && h->child[1]->type == _ASSIGNOP) {
        char temp[MAX_LENGTH];
        translateExp(h->child[0], temp, 0);
        translateExp(h->child[2], temp, 1);
        if (place != NULL) {
            if (option == 0) {
                strcpy(place, temp);
            } else {
                addIrCode(3, place, ":=", temp);
            }
        }
    } else if (h->childNum == 3 && h->child[1]->type == _LP) {
        if (place == NULL) {
            if (strcmp(h->child[0]->name, "read") == 0)
                return;
            char temp[MAX_LENGTH];
            newTemp(temp);
            addIrCode(4, temp, ":=", "CALL", h->child[0]->name);
        } else {
            if (option == 0)
                newTemp(place);
            if (strcmp(h->child[0]->name, "read") == 0)
                addIrCode(2, "READ", place);
            else
                addIrCode(4, place, ":=", "CALL", h->child[0]->name);
        }
    } else if (h->childNum == 4 && h->child[1]->type == _LP) {
        if (place == NULL) {
            if (strcmp(h->child[0]->name, "write") == 0) {
                char temp[MAX_LENGTH];
                translateExp(h->child[2]->child[0], temp, 0);
                addIrCode(2, "WRITE", temp);
            } else {
                FunDefUnit *f = getFunction(h->child[0]->name);
                translateArgs(h->child[2], f->parameterList, 0);
                char temp[MAX_LENGTH];
                newTemp(temp);
                addIrCode(4, temp, ":=", "CALL", h->child[0]->name);
            }
        } else {
            if (strcmp(h->child[0]->name, "write") == 0) {
                char temp[MAX_LENGTH];
                translateExp(h->child[2]->child[0], temp, 0);
                addIrCode(2, "WRITE", temp);
                if (option == 0) {
                    place[0] = '#';
                    place[1] = '0';
                    place[2] = '\0';
                } else {
                    addIrCode(3, place, ":=", "#0");
                }
            } else {
                FunDefUnit *f = getFunction(h->child[0]->name);
                translateArgs(h->child[2], f->parameterList, 0);
                if (option == 0)
                    newTemp(place);
                addIrCode(4, place, ":=", "CALL", h->child[0]->name);
            }
        }
    }
    if (place == NULL)return;
    if (h->child[0]->type == _INT) {
        if (option == 0) {
            sprintf(place, "#%d", h->child[0]->intValue);
        } else {
            char temp[MAX_LENGTH];
            sprintf(temp, "#%d", h->child[0]->intValue);
            addIrCode(3, place, ":=", temp);
        }
    } else if (h->child[0]->type == _ID && h->childNum == 1) {
        if (option == 0) {
            strcpy(place, h->child[0]->name);
        } else {
            addIrCode(3, place, ":=", h->child[0]->name);
        }
    } else if (h->childNum == 3 &&
               (h->child[1]->type == _PLUS || h->child[1]->type == _MINUS || h->child[1]->type == _STAR ||
                h->child[1]->type == _DIV)) {
        char temp1[MAX_LENGTH], temp2[MAX_LENGTH];
        translateExp(h->child[0], temp1, 0);
        if ((temp1[0] == '#' && temp1[1] == '0') && (h->child[1]->type == _STAR || h->child[1]->type == _DIV)) {
            if (option == 0) {
                place[0] = '#';
                place[1] = '0';
                place[2] = '\0';
            } else {
                addIrCode(3, place, ":=", "#0");
            }
        }
        translateExp(h->child[2], temp2, 0);
        if ((temp2[0] == '#' && temp2[1] == '0') && (h->child[1]->type == _STAR || h->child[1]->type == _DIV)) {
            if (option == 0) {
                place[0] = '#';
                place[1] = '0';
                place[2] = '\0';
            } else {
                addIrCode(3, place, ":=", "#0");
            }
        } else if (temp1[0] == '#' && temp2[0] == '#') {
            int i1 = strtol(&temp1[1], NULL, 10);
            int i2 = strtol(&temp2[1], NULL, 10);
            if (h->child[1]->type == _PLUS) {
                if (option == 0) {
                    sprintf(place, "#%d", i1 + i2);
                } else {
                    char temp[MAX_LENGTH];
                    sprintf(temp, "#%d", i1 + i2);
                    addIrCode(3, place, ":=", temp);
                }
            } else if (h->child[1]->type == _MINUS) {
                if (option == 0) {
                    sprintf(place, "#%d", i1 - i2);
                } else {
                    char temp[MAX_LENGTH];
                    sprintf(temp, "#%d", i1 - i2);
                    addIrCode(3, place, ":=", temp);
                }
            } else if (h->child[1]->type == _STAR) {
                if (option == 0) {
                    sprintf(place, "#%d", i1 * i2);
                } else {
                    char temp[MAX_LENGTH];
                    sprintf(temp, "#%d", i1 * i2);
                    addIrCode(3, place, ":=", temp);
                }
            } else {
                if (option == 0) {
                    sprintf(place, "#%d", i1 / i2);
                } else {
                    char temp[MAX_LENGTH];
                    sprintf(temp, "#%d", i1 / i2);
                    addIrCode(3, place, ":=", temp);
                }
            }
        } else if (temp2[0] == '#' && temp2[1] == '0') {
            if (option == 0)
                strcpy(place, temp1);
            else
                addIrCode(3, place, ":=", temp1);
        } else if (temp1[0] == '#' && temp1[1] == '0' && h->child[1]->type == _PLUS) {
            if (option == 0)
                strcpy(place, temp2);
            else
                addIrCode(3, place, ":=", temp2);
        } else {
            char op[2];
            op[1] = '\0';
            if (h->child[1]->type == _PLUS) op[0] = '+';
            else if (h->child[1]->type == _MINUS) op[0] = '-';
            else if (h->child[1]->type == _STAR) op[0] = '*';
            else op[0] = '/';
            if (option == 0) {
                newTemp(place);
            }
            addIrCode(5, place, ":=", temp1, op, temp2);
        }
    } else if (h->child[0]->type == _MINUS) {
        char temp[MAX_LENGTH];
        translateExp(h->child[1], temp, 0);
        if (temp[0] == '#') {
            int i = strtol(&temp[1], NULL, 10);
            if (option == 0) {
                sprintf(place, "#%d", -i);
            } else {
                char temp1[MAX_LENGTH];
                sprintf(temp1, "#%d", -i);
                addIrCode(3, place, ":=", temp1);
            }
        } else {
            if (option == 0) {
                newTemp(place);
            }
            addIrCode(5, place, ":=", "#0", "-", temp);
        }
    } else if (h->child[0]->type == _NOT || h->child[1]->type == _RELOP || h->child[1]->type == _AND ||
               h->child[1]->type == _OR) {
        char label1[MAX_LENGTH], label2[MAX_LENGTH];
        newLabel(label1);
        newLabel(label2);
        if (option == 0)
            newTemp(place);
        addIrCode(3, place, ":=", "#0");
        translateCond(h, label1, label2);
        addIrCode(3, "LABEL", label1, ":");
        addIrCode(3, place, ":=", "#1");
        addIrCode(3, "LABEL", label2, ":");
    } else if (h->childNum == 4 && h->child[1]->type == _LB) {
        char temp1[MAX_LENGTH], temp2[MAX_LENGTH];
        getLocation(h->child[0], temp1);
        translateExp(h->child[2], temp2, 0);
        if (temp2[0] == '#' && temp2[1] == '0') {
            char temp[MAX_LENGTH];
            if (temp1[0] == '&')
                strcpy(temp, &temp1[1]);
            else
                sprintf(temp, "*%s", temp1);
            if (option == 0) {
                strcpy(place, temp);
            } else {
                addIrCode(3, place, ":=", temp);
            }
        } else if (temp2[0] == '#') {
            int i = strtol(&temp2[1], NULL, 10);
            char temp[MAX_LENGTH], tempi[MAX_LENGTH];
            sprintf(tempi, "#%d", i * currentSize);
            newTemp(temp);
            addIrCode(5, temp, ":=", temp1, "+", tempi);
            char result[MAX_LENGTH];
            sprintf(result, "*%s", temp);
            if (option == 0)
                strcpy(place, result);
            else
                addIrCode(3, place, ":=", result);
        } else {
            char temp[MAX_LENGTH], tempi[MAX_LENGTH], result[MAX_LENGTH], num[MAX_LENGTH];
            sprintf(num, "#%d", currentSize);
            newTemp(temp);
            newTemp(tempi);
            addIrCode(5, tempi, ":=", temp2, "*", num);
            addIrCode(5, temp, ":=", temp1, "+", tempi);
            sprintf(result, "*%s", temp);
            if (option == 0)
                strcpy(place, result);
            else
                addIrCode(3, place, ":=", result);
        }
    } else if (h->childNum == 3 && h->child[1]->type == _DOT) {
        char left[MAX_LENGTH], temp[MAX_LENGTH], result[MAX_LENGTH];
        getLocation(h->child[0], left);
        int offseti = structGetOffset(currentStruct, h->child[2]->name);
        char offset[MAX_LENGTH];
        sprintf(offset, "#%d", offseti);
        newTemp(temp);
        addIrCode(5, temp, ":=", left, "+", offset);
        sprintf(result, "*%s", temp);
        if (option == 0)
            strcpy(place, result);
        else
            addIrCode(3, place, ":=", result);
    }
}

void translateCond(Node *h, char *label_true, char *label_false) {
    if (h->child[0]->type == _NOT)
        return translateCond(h->child[1], label_false, label_true);
    if (h->childNum == 3 && h->child[1]->type == _RELOP) {
        char temp1[MAX_LENGTH], temp2[MAX_LENGTH];
        translateExp(h->child[0], temp1, 0);
        translateExp(h->child[2], temp2, 0);
        if (temp1[0] == '#' && temp2[0] == '#') {
            int i1 = strtol(&temp1[1], NULL, 10);
            int i2 = strtol(&temp2[1], NULL, 10);
            if (strcmp(h->child[1]->name, ">") == 0) {
                if (i1 > i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
            if (strcmp(h->child[1]->name, "<") == 0) {
                if (i1 < i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
            if (strcmp(h->child[1]->name, ">=") == 0) {
                if (i1 >= i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
            if (strcmp(h->child[1]->name, "<=") == 0) {
                if (i1 <= i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
            if (strcmp(h->child[1]->name, "==") == 0) {
                if (i1 == i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
            if (strcmp(h->child[1]->name, "!=") == 0) {
                if (i1 != i2)
                    addIrCode(2, "GOTO", label_true);
                else
                    addIrCode(2, "GOTO", label_false);
            }
        } else {
            addIrCode(6, "IF", temp1, h->child[1]->name, temp2, "GOTO", label_true);
            addIrCode(2, "GOTO", label_false);
        }
    } else if (h->childNum == 3 && h->child[1]->type == _AND) {
        char label[MAX_LENGTH];
        newLabel(label);
        translateCond(h->child[0], label, label_false);
        addIrCode(3, "LABEL", label, ":");
        translateCond(h->child[2], label_true, label_false);
    } else if (h->childNum == 3 && h->child[1]->type == _OR) {
        char label[MAX_LENGTH];
        newLabel(label);
        translateCond(h->child[0], label_true, label);
        addIrCode(3, "LABEL", label, ":");
        translateCond(h->child[2], label_true, label_false);
    } else {
        char temp[MAX_LENGTH];
        translateExp(h, temp, 0);
        if (temp[0] == '#') {
            if (temp[1] == '0')
                addIrCode(2, "GOTO", label_false);
            else
                addIrCode(2, "GOTO", label_true);
        } else {
            addIrCode(6, "IF", temp, "!=", "#0", "GOTO", label_true);
            addIrCode(2, "GOTO", label_false);
        }
    }
}

void translateArgs(Node *h, VarDefUnit **args, int count) {
    if (h->childNum == 3)
        translateArgs(h->child[2], args, count + 1);
    if (args[count]->varType != _ARR_STRUCT_) {
        char temp[MAX_LENGTH];
        translateExp(h->child[0], temp, 0);
        addIrCode(2, "ARG", temp);
    }else {
        char temp[MAX_LENGTH];
        getLocation(h->child[0], temp);
        addIrCode(2, "ARG", temp);
    }
}

void getLocation(Node *h, char *place) {
    if (h->childNum == 1) {
        VarDefUnit *va = getValue(h->child[0]->name);
        if (va->structTypeDefUnit->arrayStructType == _STRUCT_)
            currentStruct = va->structTypeDefUnit;
        else
            currentSize = va->structTypeDefUnit->def.arrayDefList->elemSize;
        for (int i = 0; i < currentFunc->parameterNum; i++)
            if (currentFunc->parameterList[i] == va) {
                strcpy(place, h->child[0]->name);
                return;
            }
        sprintf(place, "&%s", h->child[0]->name);
    } else if (h->childNum == 4 && h->child[1]->type == _LB) {
        char temp1[MAX_LENGTH], temp2[MAX_LENGTH];
        getLocation(h->child[0], temp1);
        translateExp(h->child[2], temp2, 0);
        if (temp2[0] == '#' && temp2[1] == '0') {
            strcpy(place, temp1);
        } else if (temp2[0] == '#') {
            int i = strtol(&temp2[1], NULL, 10);
            char temp[MAX_LENGTH], tempi[MAX_LENGTH];
            newTemp(temp);
            sprintf(tempi, "#%d", i * currentSize);
            addIrCode(5, temp, ":=", temp1, "+", tempi);
            strcpy(place, temp);
        } else {
            char temp[MAX_LENGTH], tempi[MAX_LENGTH], num[MAX_LENGTH];
            sprintf(num, "#%d", currentSize);
            newTemp(temp);
            newTemp(tempi);
            addIrCode(5, tempi, ":=", temp2, "*", num);
            addIrCode(5, temp, ":=", temp1, "+", tempi);
            strcpy(place, temp);
        }
    } else {
        char left[MAX_LENGTH];
        getLocation(h->child[0], left);
        int offseti = structGetOffset(currentStruct, h->child[2]->name);
        char offset[MAX_LENGTH];
        sprintf(offset, "#%d", offseti);
        newTemp(place);
        addIrCode(5, place, ":=", left, "+", offset);
        VarDefUnit *va = getValue(h->child[2]->name);
        if (va->varType == _ARR_STRUCT_) {
            if (va->structTypeDefUnit->arrayStructType == _ARRAY_)
                currentSize = va->structTypeDefUnit->def.arrayDefList->elemSize;
            if (va->structTypeDefUnit->arrayStructType == _STRUCT_)
                currentStruct = va->structTypeDefUnit;
            else if (va->structTypeDefUnit->def.arrayDefList->elemType == _ARR_STRUCT_)
                currentStruct = va->structTypeDefUnit->def.arrayDefList->structTypeDefUnit;
        }
    }
}