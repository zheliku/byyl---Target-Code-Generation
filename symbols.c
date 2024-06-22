#include "symbols.h"

//用于取名的变量和定义
#define name_head "__compiler_exp"
#define name_tail "__"
static int nameCount = 0;

//符号表主表。本实验只有一张符号表。
struct {
    TypeDefUnit *types;
    FunDefUnit *funcs;
    ValueStack *values;
} symbols;

//强制出栈，不带保存。
void valueStackPopViolent();

//复制网上的int转str函数
void myItoa(unsigned long val, char *buf, unsigned radix) {
    char *p;                    /* pointer to traverse string */
    char *firstdig;                /* pointer to first digit */
    char temp;                    /* temp char */
    unsigned digval;            /* value of digit */

    p = buf;
    firstdig = p;                /* save pointer to first digit */

    do {
        digval = (unsigned) (val % radix);
        val /= radix;            /* get next digit */

        /* convert to ascii and store */
        if (digval > 9)
            *p++ = (char) (digval - 10 + 'a');    /* arrayDefList letter */
        else
            *p++ = (char) (digval + '0');        /* arrayDefList digit */
    } while (val > 0);

    /* We now have the digit of the number in the buffer, but in reverse
     * order. Thus we reverse them now. */

    *p-- = '\0';                /* terminate string; p points to last digit */

    do {
        temp = *p;
        *p = *firstdig;
        *firstdig = temp;        /* swap *p and *firstdig */
        --p;
        ++firstdig;                /* advance to next two digits */
    } while (firstdig < p);        /* repeat until halfway */
}

void getAName(char *name) {
    strcpy(name, name_head);
    char temp[20];
    myItoa(nameCount, temp, 10);
    nameCount++;
    strcat(name, temp);
    strcat(name, name_tail);
}

void initSymbolTable() {
    symbols.types = NULL;
    symbols.funcs = NULL;
    symbols.values = NULL;
    valueStackPush();
    FunDefUnit *f_read = newFunction("read");
    addFunctionDeclaration(f_read);
    FunDefUnit *f_write = newFunction("write");
    addFunctionDeclaration(f_write);
    f_read->returnType = _INT_;
    f_write->returnType = _INT_;
    f_write->parameterNum = 1;
    f_write->varType = (VarType *) malloc(sizeof(VarType));
    f_write->parameters = (TypeDefUnit **) malloc(sizeof(VarDefUnit *));
    f_write->varType[0] = _INT_;
    f_write->parameters[0] = NULL;
}

void destroySymbolTable() {
    while (symbols.values != NULL)
        valueStackPopViolent();
    {
        FunDefUnit *p;
        while (symbols.funcs != NULL) {
            p = symbols.funcs;
            symbols.funcs = symbols.funcs->next;
            free(p->varType);
            free(p->parameters);
            free(p->parameterList);
            free(p);
        }
    }
    {
        TypeDefUnit *p;
        while (symbols.types != NULL) {
            p = symbols.types;
            symbols.types = symbols.types->next;
            if (p->arrayStructType == _STRUCT_) {
                free(p->def.structDefList->defList);
                free(p->def.structDefList);
            } else {
                ArrayDefList *q;
                while (p->def.arrayDefList != NULL) {
                    q = p->def.arrayDefList;
                    p->def.arrayDefList = p->def.arrayDefList->next;
                    free(q);
                }
            }
            free(p);
        }
    }
}

TypeDefUnit *getType(const char *name) {
    TypeDefUnit *p = symbols.types;
    while (p != NULL) {
        if (p->arrayStructType == _STRUCT_ && strcmp(name, p->name) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}

FunDefUnit *getFunction(const char *name) {
    FunDefUnit *p = symbols.funcs;
    while (p != NULL) {
        if (strcmp(name, p->name) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}

VarDefUnit *getValue(const char *name) {
    ValueStack *q = symbols.values;
    while (q != NULL) {
        VarDefUnit *p = q->values;
        while (p != NULL) {
            if (strcmp(name, p->name) == 0)
                return p;
            p = p->next;
        }
        q = q->next;
    }
    return NULL;
}

TypeDefUnit *addTypeDeclaration(TypeDefUnit *r) {
    insert_head(symbols.types, r);
    return r;
}

FunDefUnit *addFunctionDeclaration(FunDefUnit *r) {
    insert_head(symbols.funcs, r);
    return r;
}

VarDefUnit *addValueDeclaration(VarDefUnit *r) {
    insert_head(symbols.values->values, r);
    return r;
}

TypeDefUnit *newType(const char *name) {
    TypeDefUnit *p = (TypeDefUnit *) malloc(sizeof(TypeDefUnit));
    p->next = NULL;
    if (name != NULL) {
        strcpy(p->name, name);
        p->arrayStructType = _STRUCT_;
        p->def.structDefList = (StructDefList *) malloc(sizeof(StructDefList));
        p->def.structDefList->defNum = 0;
        p->def.structDefList->defList = NULL;
    } else {
        p->name[0] = '\0';
        p->arrayStructType = _ARRAY_;
        p->def.arrayDefList = NULL;
    }
    return p;
}

FunDefUnit *newFunction(const char *name) {
    FunDefUnit *p = (FunDefUnit *) malloc(sizeof(FunDefUnit));
    p->next = NULL;
    strcpy(p->name, name);
    p->parameterNum = 0;
    p->parameters = NULL;
    p->varType = NULL;
    p->returnTypeDefUnit = NULL;
    p->parameterList = NULL;
    return p;
}

VarDefUnit *newValue(const char *name) {
    VarDefUnit *p = (VarDefUnit *) malloc(sizeof(VarDefUnit));
    strcpy(p->name, name);
    p->next = NULL;
    p->usable = 1;
    p->varType = _INT_;
    p->structTypeDefUnit = NULL;
    return p;
}

void arrayGenerateBasicDimension(TypeDefUnit *t, int number, VarType kind, TypeDefUnit *val_type) {
    t->def.arrayDefList = (ArrayDefList *) malloc(sizeof(ArrayDefList));
    t->def.arrayDefList->dimension = 0;
    t->def.arrayDefList->number = number;
    if (kind != _ARR_STRUCT_)
        t->def.arrayDefList->elemSize = 4;
    else
        t->def.arrayDefList->elemSize = structGetSize(val_type);
    t->def.arrayDefList->elemType = kind;
    t->def.arrayDefList->structTypeDefUnit = val_type;
    t->def.arrayDefList->next = NULL;
}

void arrayExpandDimension(TypeDefUnit *t, int number) {
    ArrayDefList *p = (ArrayDefList *) malloc(sizeof(ArrayDefList));
    p->dimension = t->def.arrayDefList->dimension + 1;
    p->number = number;
    p->elemSize = (t->def.arrayDefList->number) * (t->def.arrayDefList->elemSize);
    p->elemType = t->def.arrayDefList->elemType;
    p->structTypeDefUnit = t->def.arrayDefList->structTypeDefUnit;
    p->next = NULL;
    insert_head(t->def.arrayDefList, p);
}

int typeEqual(TypeDefUnit *p, TypeDefUnit *q) {
    if (p == q)
        return 1;
    if (p->arrayStructType != q->arrayStructType)
        return 0;
    if (p->arrayStructType == _STRUCT_)
        return 0;
    if (p->def.arrayDefList->dimension == q->def.arrayDefList->dimension &&
        p->def.arrayDefList->elemType == q->def.arrayDefList->elemType &&
        p->def.arrayDefList->structTypeDefUnit == q->def.arrayDefList->structTypeDefUnit)
        return 1;
    return 0;
}

void valueStackPush() {
    ValueStack *p = (ValueStack *) malloc(sizeof(ValueStack));
    p->values = NULL;
    p->next = NULL;
    insert_head(symbols.values, p);
}

void valueStackPop() {
    VarDefUnit *p;
    ValueStack *q = symbols.values;
    symbols.values = symbols.values->next;
    while (q->values != NULL) {
        p = q->values;
        q->values = q->values->next;
        /*if(p->usable)
            free(p);
        else*/
        insert_head(symbols.values->values, p);
    }
    free(q);
}

void valueStackPopViolent() {
    VarDefUnit *p;
    ValueStack *q = symbols.values;
    symbols.values = symbols.values->next;
    while (q->values != NULL) {
        p = q->values;
        q->values = q->values->next;
        free(p);
    }
    free(q);
}

int valueStackCheck(const char *name) {
    VarDefUnit *p = symbols.values->values;
    while (p != NULL) {
        if (strcmp(p->name, name) == 0)
            return 0;
        p = p->next;
    }
    TypeDefUnit *temp = getType(name);
    if (temp == NULL)
        return 1;
    return 0;
}

int structGetSize(TypeDefUnit *s) {
    if (s->arrayStructType == _ARRAY_)
        return (s->def.arrayDefList->number) * (s->def.arrayDefList->elemSize);
    StructDefList *p = s->def.structDefList;
    VarDefUnit *q;
    int count = 0;
    for (int i = 0; i < p->defNum; i++) {
        q = p->defList[i];
        if (q->varType != _ARR_STRUCT_)
            count += 4;
        else
            count += structGetSize(q->structTypeDefUnit);
    }
    return count;
}

int structGetOffset(TypeDefUnit *s, char *field) {
    VarDefUnit *field_ptr = getValue(field);
    StructDefList *p = s->def.structDefList;
    VarDefUnit *q;
    int count = 0;
    for (int i = 0; i < p->defNum; i++) {
        q = p->defList[i];
        if (q == field_ptr)break;
        if (q->varType != _ARR_STRUCT_)
            count += 4;
        else
            count += structGetSize(q->structTypeDefUnit);
    }
    return count;
}