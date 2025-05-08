// src/prs/bdd2prs.h

#ifndef BDD2PRS_H
#define BDD2PRS_H

#include <stdbool.h>

int Bdd2Prs(Abc_Ntk_t * pNtk, int fReorder);

typedef struct {
    DdNode *input;
    bool isTBranch; // true = Then, false = Else
} InputEdge_t;

typedef struct {
    DdNode *node;
    const char *varName;
    int nodeIndex;
} NodeVarEntry_t;



#endif