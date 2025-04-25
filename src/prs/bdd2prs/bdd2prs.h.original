// src/prs/bdd2prs.h

#ifndef BDD2PRS_H
#define BDD2PRS_H

int Bdd2Prs(Abc_Ntk_t * pNtk, int fReorder);

typedef struct BddCondition_t {
    char *label;
    DdNode *node;
    DdNode *T;
    DdNode *E;
} BddCondition_t;

typedef struct {
    BddCondition_t *items;
    int size;
    int capacity;
} CondList;

#endif