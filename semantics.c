#include "semantics.h"
#include "symbols.h"
#include "log.h"

static VarType varType = _ARR_STRUCT_;            //当前处理类型：int，float，用户定义类型
static TypeDefUnit *pTypeDefUnit = NULL;          //当前处理的用户定义类型的定义结构体指针（如果需要）
static FunDefUnit *pFunDefUnit = NULL;            //当前正在处理的函数定义指针。
static int paraCount = 0;                         //参数数目。
static VarDefUnit *paras[512];                    //各参数定义。不想用链表了。一个结构体，一个函数的变量、参数不超过512个。
static int needCount = 0;                         //是否需要记录定义的变量。
static int structing = 0;                         //是否正在定义结构体，用于标志当前变量定义是否是结构体内的域定义
static int doNotPush = 0;                         //提醒在函数刚建立的CompSt不需要push符号表。
static VarDefUnit *last_val = NULL;               //最近定义的变量。
static int errorOccured = 0;

static Stack *stHead = NULL;

void dealWith(Node *h) {
    if (h == NULL)return;
    switch (h->type) {
        case Specifier: {
            dealWithSpecifier(h);
            break;
        }
        case StructSpecifier: {
            dealWithStructSpecifier(h);
            break;
        }
        case CompSt: {
            dealWithCompSt(h);
            break;
        }
        case ExtDef: {
            dealWithExtDef(h);
            break;
        }
        case VarDec: {
            dealWithVarDec(h);
            break;
        }
        case Stmt: {
            dealWithStmt(h);
            break;
        }
        case Dec: {
            dealWithDec(h);
            break;
        }
        default: {
            for (int i = 0; i < h->childNum; i++)
                dealWith(h->child[i]);
            break;
        }
    }
}


void dealWithSpecifier(Node *h) {
    //在这里获得当前处理的类型，更改全局变量
    if (h->child[0]->type == _TYPE) {
        if (strcmp(h->child[0]->name, "int") == 0)varType = _INT_;
        else varType = _FLOAT_;
        pTypeDefUnit = NULL;
    } else {
        varType = _ARR_STRUCT_;
        dealWith(h->child[0]);
    }
}

void dealWithStructSpecifier(Node *h) {
    if (h->childNum == 2) {
        //这里是使用已经定义的结构体
        pTypeDefUnit = getType(h->child[1]->child[0]->name);
        if (pTypeDefUnit == NULL) {
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 17 at Line %d: undefined struct %s.\n", h->lineNum,
                    h->child[1]->child[0]->name);
            reportError(SemanticError, message);
            errorOccured = 1;
        }
    } else {
        //定义结构体，可能会直接使用于定义变量，可以使匿名结构体定义。
        char name[33];
        if (h->child[1]->child[0]->type == None)
            getAName(name);
        else
            strcpy(name, h->child[1]->child[0]->name);
        pTypeDefUnit = getType(name);
        if (pTypeDefUnit != NULL || getValue(name) != NULL) {
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 16 at Line %d: name %s is used.\n", h->lineNum, name);
            reportError(SemanticError, message);
            errorOccured = 1;
            pTypeDefUnit = NULL;
        } else {
            pTypeDefUnit = newType(name);
            addTypeDeclaration(pTypeDefUnit);
            push();
            structing = 1;
            needCount = 1;
            paraCount = 0;
            dealWith(h->child[3]);
            stHead->structTypeDefUnit->def.structDefList->defNum = paraCount;
            stHead->structTypeDefUnit->def.structDefList->defList = (VarDefUnit **) malloc(
                    sizeof(VarDefUnit *) * paraCount);
            for (int i = 0; i < paraCount; i++)
                stHead->structTypeDefUnit->def.structDefList->defList[i] = paras[i];
            needCount = 0;
            structing = 0;
            pop();
        }
    }
}

void dealWithCompSt(Node *h) {
    if (doNotPush)
        doNotPush = 0;
    else
        valueStackPush();
    dealWith(h->child[1]);
    dealWith(h->child[2]);
    valueStackPop();
}

void dealWithExtDef(Node *h) {
    /* 此处是定义，依据第二个子结点是ExtDecList,SEMI,FunDec,
    * 分别确定是定义全局变量、函数的定义，分别作不同处理
    */
    if (h->child[1]->type == FunDec) {
        dealWith(h->child[0]);
        FunDefUnit *temp = getFunction(h->child[1]->child[0]->name);
        if (temp != NULL) {
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 4 at Line %d: function %s is redefined.\n", h->lineNum,
                    h->child[1]->child[0]->name);
            reportError(SemanticError, message);
            errorOccured = 1;
        } else {
            temp = newFunction(h->child[1]->child[0]->name);
            addFunctionDeclaration(temp);
            pFunDefUnit = temp;
            temp->returnTypeDefUnit = pTypeDefUnit;
            temp->returnType = varType;
            valueStackPush();
            doNotPush = 1;
            needCount = 1;
            paraCount = 0;
            dealWith(h->child[1]->child[2]);
            temp->parameterNum = paraCount;
            if (paraCount != 0) {
                temp->varType = (VarType *) malloc(sizeof(VarType) * paraCount);
                temp->parameters = (TypeDefUnit **) malloc(sizeof(TypeDefUnit *) * paraCount);
                temp->parameterList = (VarDefUnit **) malloc(sizeof(VarDefUnit *) * paraCount);
            }
            for (int i = 0; i < paraCount; i++) {
                temp->varType[i] = paras[i]->varType;
                temp->parameters[i] = paras[i]->structTypeDefUnit;
                temp->parameterList[i] = paras[i];
            }
            needCount = 0;
            dealWith(h->child[2]);
            doNotPush = 0;
            pFunDefUnit = NULL;
        }
    } else
        for (int i = 0; i < h->childNum; i++)
            dealWith(h->child[i]);
}

void dealWithVarDec(Node *h) {
    Node *temp = h;
    while (temp->childNum != 1)
        temp = temp->child[0];
    temp = temp->child[0];
    int check = valueStackCheck(temp->name);
    if (!check) {
        VarDefUnit *che = getValue(temp->name);
        int i = -1;
        if (structing)
            for (i = 0; i < paraCount; i++)
                if (paras[i] == che)
                    break;
        if (i == paraCount || i == -1) {
            errorOccured = 1;
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 3 at Line %d: variable %s is redefined.\n", h->lineNum, temp->name);
            reportError(SemanticError, message);
        } else {
            errorOccured = 1;
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 15 at Line %d: Redefined field %s.\n", h->lineNum, temp->name);
            reportError(SemanticError, message);
        }
        last_val = NULL;
    } else if (!(varType == _ARR_STRUCT_ && pTypeDefUnit == NULL)) {
        VarDefUnit *v = newValue(temp->name);
        v->usable = !structing;
        if (h->childNum == 1) {
            v->varType = varType;
            v->structTypeDefUnit = pTypeDefUnit;
        } else {
            v->varType = _ARR_STRUCT_;
            v->structTypeDefUnit = newType(NULL);
            addTypeDeclaration(v->structTypeDefUnit);
            temp = h->child[0];
            arrayGenerateBasicDimension(v->structTypeDefUnit, h->child[2]->intValue, varType, pTypeDefUnit);
            while (temp->childNum != 1) {
                arrayExpandDimension(v->structTypeDefUnit, temp->child[2]->intValue);
                temp = temp->child[0];
            }
        }
        addValueDeclaration(v);
        last_val = v;
        if (needCount) {
            paras[paraCount] = v;
            paraCount++;
        }
    }
}

void dealWithStmt(Node *h) {
    //根据第一个子结点的不同，分别作不同的处理
    switch (h->child[0]->type) {
        case Exp: {
            VarType temp1;
            TypeDefUnit *temp2;
            dealWithExp(&temp1, &temp2, h->child[0]);
            break;
        }
        case CompSt: {
            dealWith(h->child[0]);
            break;
        }
        case _RETURN: {
            VarType temp1;
            TypeDefUnit *temp2;
            dealWithExp(&temp1, &temp2, h->child[1]);
            if (!(temp1 == _ARR_STRUCT_ && temp2 == NULL))
                if (pFunDefUnit->returnType != temp1 || pFunDefUnit->returnTypeDefUnit != temp2) {
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 8 at Line %d: invalid return type\n", h->lineNum);
                    reportError(SemanticError, message);
                    errorOccured = 1;
                }
            break;
        }
        default: {
            VarType temp1;
            TypeDefUnit *temp2;
            dealWithExp(&temp1, &temp2, h->child[2]);
            if (temp1 != _INT_ && !(temp1 == _ARR_STRUCT_ && temp2 == NULL)) {
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 7 at Line %d: only int can be used as boolean\n", h->lineNum);
                reportError(SemanticError, message);
                errorOccured = 1;
            }
            for (int i = 4; i < h->childNum; i++)
                dealWith(h->child[i]);
            break;
        }
    }
}

void dealWithDec(Node *h) {
    dealWith(h->child[0]);
    if (h->childNum == 3) {
        if (structing) {
            errorOccured = 1;
            char message[MESSAGE_LENGTH];
            sprintf(message,
                    "Error type 15 at Line %d: can't initialize arrayDefList field while defining the struct\n",
                    h->lineNum);
            reportError(SemanticError, message);
        } else {
            VarType temp1;
            TypeDefUnit *temp2;
            dealWithExp(&temp1, &temp2, h->child[2]);
            if (!(temp1 == _ARR_STRUCT_ && temp2 == NULL) && last_val != NULL)
                if (last_val->varType != temp1 || !typeEqual(last_val->structTypeDefUnit, temp2)) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 5 at Line %d: incompatible type near =\n", h->lineNum);
                    reportError(SemanticError, message);
                }
        }
    }
}


void dealWithExp(VarType *varType, TypeDefUnit **pTypeDefUnit, Node *h) {
    if (h->childNum == 1) {
        /* ID
         * INT
         * FLOAT
         */
        switch (h->child[0]->type) {
            case _ID: {
                VarDefUnit *temp = (VarDefUnit *) checkId(h->child[0], _IdVal_);
                if (temp != NULL) {
                    if (temp->usable == 0) {
                        errorOccured = 1;
                        char message[MESSAGE_LENGTH];
                        sprintf(message, "Error type 1 at Line %d: %s is arrayDefList field of arrayDefList struct\n",
                                h->lineNum, temp->name);
                        reportError(SemanticError, message);
                        *varType = _ARR_STRUCT_;
                        *pTypeDefUnit = NULL;
                    } else {
                        *varType = temp->varType;
                        *pTypeDefUnit = temp->structTypeDefUnit;
                    }
                } else {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                }
                break;
            }
            case _INT: {
                *varType = _INT_;
                *pTypeDefUnit = NULL;
                break;
            }
            case _FLOAT: {
                *varType = _FLOAT_;
                *pTypeDefUnit = NULL;
                break;
            }
        }
    } else if (h->childNum == 2) {
        /* NOT Exp
         * MINUS Exp
         */
        VarType temp1;
        TypeDefUnit *temp2;
        dealWithExp(&temp1, &temp2, h->child[1]);
        if (h->child[0]->type == _NOT) {
            if ((temp1 == _ARR_STRUCT_ && temp2 != NULL) || temp1 == _FLOAT_) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 7 at Line %d: only int can use \"!\"(not)\n", h->lineNum);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
            } else {
                *varType = temp1;
                *pTypeDefUnit = temp2;
            }
        } else {
            if (temp1 == _ARR_STRUCT_ && temp2 != NULL) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 7 at Line %d: only int or float can use \"-\"(minus)\n", h->lineNum);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
            } else {
                *varType = temp1;
                *pTypeDefUnit = temp2;
            }
        }
    } else if (h->childNum == 3) {
        /*
         *  Exp ASSIGNOP Exp
         *  Exp AND Exp
         *  Exp OR Exp
         *  Exp (cal) Exp : cal requires both EXPs are int or (both are) float
         *  LP Exp RP
         *  ID LP RP
         *  Exp DOT ID
         */
        switch (h->child[1]->type) {
            case Exp: {
                dealWithExp(varType, pTypeDefUnit, h->child[1]);
                break;
            }
            case _LP: {
                if (getValue(h->child[0]->name) != NULL) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message,
                            "Error type 11 at Line %d: %s is arrayDefList variable, not arrayDefList function\n",
                            h->lineNum, h->child[0]->name);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    return;
                }
                FunDefUnit *temp = (FunDefUnit *) checkId(h->child[0], _IdFunc_);
                if (temp == NULL) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else if (temp->parameterNum == 0) {
                    *varType = temp->returnType;
                    *pTypeDefUnit = temp->returnTypeDefUnit;
                } else {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 9 at Line %d: unmatched parameters for function1 %s\n", h->lineNum,
                            temp->name);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                }
                break;
            }
            case _DOT: {
                VarType temp1;
                TypeDefUnit *temp2;
                dealWithExp(&temp1, &temp2, h->child[0]);
                if (temp1 != _ARR_STRUCT_) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 13 at Line %d: use \".\" on none struct variable\n", h->lineNum);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else if (temp2 == NULL) {
                    errorOccured = 1;
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else if (temp2->arrayStructType == _ARRAY_) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 13 at Line %d: use \".\" on none struct variable\n", h->lineNum);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else {
                    VarDefUnit *temp = getValue(h->child[2]->name);
                    for (int i = 0; i < temp2->def.structDefList->defNum; i++)
                        if (temp2->def.structDefList->defList[i] == temp) {
                            *varType = temp->varType;
                            *pTypeDefUnit = temp->structTypeDefUnit;
                            return;
                        }
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 14 at Line %d: undefined field %s for struct %s\n", h->lineNum,
                            h->child[2]->name, temp2->name);
                    reportError(SemanticError, message);
                }
                break;
            }
            case _AND:
            case _OR: {
                VarType temp1, temp3;
                TypeDefUnit *temp2, *temp4;
                dealWithExp(&temp1, &temp2, h->child[0]);
                dealWithExp(&temp3, &temp4, h->child[2]);
                if ((temp1 == _ARR_STRUCT_ && temp2 == NULL) || (temp3 == _ARR_STRUCT_ && temp4 == NULL)) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else if (temp1 != _INT_ || temp3 != _INT_) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 7 at Line %d: only int can be used as boolean\n", h->lineNum);
                    reportError(SemanticError, message);
                } else {
                    *varType = _INT_;
                    *pTypeDefUnit = NULL;
                }
                break;
            }
            case _ASSIGNOP: {
                if (checkLeft(h->child[0]) == 0) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 6 at Line %d: the left-hand side of \"=\" must have left side value\n",
                            h->lineNum);
                    reportError(SemanticError, message);
                } else {
                    VarType temp1, temp3;
                    TypeDefUnit *temp2, *temp4;
                    dealWithExp(&temp1, &temp2, h->child[0]);
                    dealWithExp(&temp3, &temp4, h->child[2]);
                    if ((temp1 == _ARR_STRUCT_ && temp2 == NULL) || (temp3 == _ARR_STRUCT_ && temp4 == NULL)) {
                        *varType = _ARR_STRUCT_;
                        *pTypeDefUnit = NULL;
                    } else if (temp1 == temp3 && typeEqual(temp2, temp4)) {
                        *varType = temp1;
                        *pTypeDefUnit = temp2;
                    } else {
                        *varType = _ARR_STRUCT_;
                        *pTypeDefUnit = NULL;
                        errorOccured = 1;
                        char message[MESSAGE_LENGTH];
                        sprintf(message, "Error type 5 at Line %d: both sides of \"=\" must be the same type\n",
                                h->lineNum);
                        reportError(SemanticError, message);
                    }
                }
                break;
            }
            default: {
                VarType temp1, temp3;
                TypeDefUnit *temp2, *temp4;
                dealWithExp(&temp1, &temp2, h->child[0]);
                dealWithExp(&temp3, &temp4, h->child[2]);
                if ((temp1 == _ARR_STRUCT_ && temp2 == NULL) || (temp3 == _ARR_STRUCT_ && temp4 == NULL)) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                } else if (temp1 == temp3 && (temp1 == _INT_ || temp1 == _FLOAT_)) {
                    *varType = temp1;
                    *pTypeDefUnit = NULL;
                } else {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message,
                            "Error type 7 at Line %d: calculation requires both EXPs are int or (both are) float\n",
                            h->lineNum);
                    reportError(SemanticError, message);
                }
                break;
            }
        }
    } else if (h->childNum == 4) {
        /* Exp LB Exp RB
         * ID LP Args RP
         */
        if (h->child[0]->type == Exp) {
            //找到头部数组变量的定义
            Node *head = h;
            while (head->childNum == 4 && head->child[0]->type == Exp) {
                head = head->child[0];
            }
            if (!(head->childNum == 1 && head->child[0]->type == _ID)) {
                errorOccured = 0;
                //sprintf(message, "Error type 10 at Line %d: use \"[]\" on none array variable\n",h->lineNum);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            VarDefUnit *temp = (VarDefUnit *) checkId(head->child[0], _IdVal_);
            if (temp == NULL) {
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            if (temp->varType != _ARR_STRUCT_ || temp->structTypeDefUnit->arrayStructType != _ARRAY_) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 10 at Line %d: use \"[]\" on none array variable\n", h->lineNum);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            //顺次偏移，一个[]对应数组定义链表一层
            ArrayDefList *current = temp->structTypeDefUnit->def.arrayDefList;
            head = head->father;
            while (head != h && current->dimension != 0) {
                //检查[]内数字合法性
                VarType temp1;
                TypeDefUnit *temp2;
                dealWithExp(&temp1, &temp2, head->child[2]);
                if (temp1 == _ARR_STRUCT_ && temp2 == NULL) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    return;
                } else if (temp1 != _INT_) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 12 at Line %d: the type of exp between \"[]\" should be int\n",
                            h->lineNum);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    return;
                }
                head = head->father;
                current = current->next;
            }
            if (current->dimension == 0 && head != h) {
                //[]过多
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 10 at Line %d: use \"[]\" on none array variable\n", h->lineNum);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            //检查最后一个[]内数字的合法性
            VarType temp1;
            TypeDefUnit *temp2;
            dealWithExp(&temp1, &temp2, head->child[2]);
            if (temp1 == _ARR_STRUCT_ && temp2 == NULL) {
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            } else if (temp1 != _INT_) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 12 at Line %d: the type of exp between \"[]\" should be int\n",
                        h->lineNum);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            if (head == h && current->dimension == 0) {
                //刚好对齐，取出基本元素定义
                *varType = current->elemType;
                *pTypeDefUnit = current->structTypeDefUnit;
                return;
            } else {
                //取得部分数组。复制，新建一个对应的类型返回上层。
                TypeDefUnit *p = newType(NULL);
                addTypeDeclaration(p);
                current = current->next;
                ArrayDefList *copy_current = p->def.arrayDefList;
                while (current != NULL) {
                    ArrayDefList *copy_temp = (ArrayDefList *) malloc(sizeof(ArrayDefList));
                    memcpy(copy_temp, current, sizeof(ArrayDefList));
                    copy_temp->next = NULL;
                    if (copy_current == NULL)
                        p->def.arrayDefList = copy_temp;
                    else
                        copy_current->next = copy_temp;
                    copy_current = copy_temp;
                    current = current->next;
                }
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = p;
            }
        } else {
            if (getValue(h->child[0]->name) != NULL) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 11 at Line %d: %s is arrayDefList variable, not arrayDefList function\n",
                        h->lineNum, h->child[0]->name);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            FunDefUnit *temp = (FunDefUnit *) checkId(h->child[0], _IdFunc_);
            if (temp == NULL) {
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            //计算args里面有几个参数
            Node *args_temp = h->child[2];
            int count = 1;
            while (args_temp->childNum == 3) {
                count++;
                args_temp = args_temp->child[2];
            }
            if (count != temp->parameterNum) {
                errorOccured = 1;
                char message[MESSAGE_LENGTH];
                sprintf(message, "Error type 9 at Line %d: unmatched parameters for function2 %s\n", h->lineNum,
                        temp->name);
                reportError(SemanticError, message);
                *varType = _ARR_STRUCT_;
                *pTypeDefUnit = NULL;
                return;
            }
            //依次检查各个参数
            args_temp = h->child[2];
            for (int i = 0; i < count; i++) {
                VarType temp1;
                TypeDefUnit *temp2;
                dealWithExp(&temp1, &temp2, args_temp->child[0]);
                if (temp1 == _ARR_STRUCT_ && temp2 == NULL) {
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    return;
                }
                if(temp2!=NULL && temp->parameters[i]!=NULL)
                {
                    return;
                }

                if (temp1 != temp->varType[i] || temp2 != temp->parameters[i]) {
                    errorOccured = 1;
                    char message[MESSAGE_LENGTH];
                    sprintf(message, "Error type 9 at Line %d: unmatched parameters for function3 %s\n", h->lineNum,
                            temp->name);
                    reportError(SemanticError, message);
                    *varType = _ARR_STRUCT_;
                    *pTypeDefUnit = NULL;
                    return;
                }
                if (args_temp->childNum == 3)
                    args_temp = args_temp->child[2];
            }
            *varType = temp->returnType;
            *pTypeDefUnit = temp->returnTypeDefUnit;
        }
    }
}

void *checkId(Node *h, idType identity) {
    if (identity == _IdFunc_) {
        FunDefUnit *temp = getFunction(h->name);
        if (temp == NULL) {
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 2 at Line %d: undefined function %s\n", h->lineNum, h->name);
            reportError(SemanticError, message);
            errorOccured = 1;
        }
        return temp;
    } else {
        VarDefUnit *temp = getValue(h->name);
        if (temp == NULL) {
            errorOccured = 1;
            char message[MESSAGE_LENGTH];
            sprintf(message, "Error type 1 at Line %d: undefined variable %s\n", h->lineNum, h->name);
            reportError(SemanticError, message);
        }
        return temp;
    }
}

int checkLeft(Node *h) {
    if (h->childNum == 1 && h->child[0]->type == _ID)
        return 1;
    if (h->childNum == 3 && h->child[1]->type == _DOT)
        return 1;
    if (h->childNum == 4 && h->child[1]->type == _LB)
        return 1;
    return 0;
}

int getErrorOccured() {
    return errorOccured;
}

void push() {
    Stack *p = (Stack *) malloc(sizeof(Stack));
    p->type = varType;
    p->structTypeDefUnit = pTypeDefUnit;
    p->paraNum = paraCount;
    p->needNum = needCount;
    p->structing = structing;
    for (int i = 0; i < paraCount; i++)
        p->paras[i] = paras[i];
    paraCount = 0;
    insert_head(stHead, p);
}

void pop() {
    Stack *p = stHead;
    stHead = stHead->next;
    varType = p->type;
    pTypeDefUnit = p->structTypeDefUnit;
    paraCount = p->paraNum;
    structing = p->structing;
    for (int i = 0; i < p->paraNum; i++)
        paras[i] = p->paras[i];
    needCount = p->needNum;
    free(p);
}