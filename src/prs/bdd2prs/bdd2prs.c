// src/prs/bdd2prs.c

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "base/abc/abc.h"
#include "bdd/cudd/cudd.h"
#include "bdd/cudd/cuddInt.h"
#include "bdd2prs.h"
// #include "vec.h"

// void PrintPointerChainsFromLeaf(st__table *inputMap, DdNode *current, Vec_Ptr_t *path) {
//     Vec_PtrPush(path, current);

//     Vec_Ptr_t *inputs = NULL;
//     if (!st__lookup(inputMap, (char *)current, (char **)&inputs) || Vec_PtrSize(inputs) == 0) {
//         // Reached a root node, print chain
//         for (int i = Vec_PtrSize(path) - 1; i >= 0; i--) {
//             printf("%p", (void *)Vec_PtrEntry(path, i));
//             if (i > 0)
//                 printf(" -> ");
//         }
//         printf("\n");
//     } else {
//         for (int i = 0; i < Vec_PtrSize(inputs); i++) {
//             DdNode *input = (DdNode *)Vec_PtrEntry(inputs, i);
//             PrintPointerChainsFromLeaf(inputMap, input, path);
//         }
//     }

//     Vec_PtrPop(path);
// }

// void PrintPointerChainsFromLeaf(st__table *inputMap, DdNode *current, Vec_Ptr_t *path) {
//     Vec_PtrPush(path, current);

//     Vec_Ptr_t *inputs = NULL;
//     if (!st__lookup(inputMap, (char *)current, (char **)&inputs) || Vec_PtrSize(inputs) == 0) {
//         // Reached a root node — print the full path
//         for (int i = Vec_PtrSize(path) - 1; i >= 0; i--) {
//             printf("%p", Vec_PtrEntry(path, i));
//             if (i > 0) printf(" -> ");
//         }
//         printf("\n");
//     } else {
//         for (int i = 0; i < Vec_PtrSize(inputs); i++) {
//             InputEdge_t *edge = (InputEdge_t *)Vec_PtrEntry(inputs, i);
//             if (edge->input == Cudd_Not(0)) continue; // Skip ELSE to 0
//             PrintPointerChainsFromLeaf(inputMap, edge->input, path);
//         }
//     }

//     Vec_PtrPop(path);
// }

void PrintPointerChainsFromLeaf(st__table *inputMap, DdNode *current, Vec_Ptr_t *path, bool isTBranch) {
    // Create an edge struct to record how we got to this node
    InputEdge_t *edge = ABC_ALLOC(InputEdge_t, 1);
    edge->input = current;
    edge->isTBranch = isTBranch;
    Vec_PtrPush(path, edge);

    Vec_Ptr_t *inputs = NULL;
    if (!st__lookup(inputMap, (char *)current, (char **)&inputs) || Vec_PtrSize(inputs) == 0) {
        // Reached a root node — print the full path
        for (int i = Vec_PtrSize(path) - 1; i >= 0; i--) {
            InputEdge_t *e = (InputEdge_t *)Vec_PtrEntry(path, i);
            printf("%p [%c]", (void *)e->input, e->isTBranch ? 'T' : 'E');
            if (i > 0) printf(" -> ");
        }
        printf("\n");
    } else {
        for (int i = 0; i < Vec_PtrSize(inputs); i++) {
            InputEdge_t *nextEdge = (InputEdge_t *)Vec_PtrEntry(inputs, i);
            if (nextEdge->input == Cudd_Not(0)) continue; // Skip ELSE to 0
            PrintPointerChainsFromLeaf(inputMap, nextEdge->input, path, nextEdge->isTBranch);
        }
    }

    Vec_PtrPop(path);
    ABC_FREE(edge);
}






void TraverseWithCudd(DdManager *dd, DdNode *f, Abc_Ntk_t *pNtk, int iPo) {
    DdGen *gen;
    DdNode *node;

    Abc_Obj_t *pPo = Abc_NtkCo(pNtk, iPo);
    const char *outputName = Abc_ObjName(pPo);
    DdNode *outputPointer = f;

    Vec_Ptr_t *vNodeVarMap = Vec_PtrAlloc(100); // Initial Capacity of 100

    st__table *nodeInputMap = st__init_table(st__ptrcmp, st__ptrhash);
    st__table *nodeMap = st__init_table(st__ptrcmp, st__ptrhash);
    int nodeCounter = 1000;


    printf("\n>>> Traversing output %s [%p]\n", outputName, outputPointer);

    // Ensure the function is in regular form (remove complement edge if any)
    node = Cudd_Regular(f);

    // Create a generator to iterate over nodes
    if (!(gen = Cudd_FirstNode(dd, node, &node))) {
        printf("Cudd_FirstNode failed.\n");
        return;
    }

    DdNode *zero = Cudd_ReadLogicZero(dd);
    printf("ZERO pointer: %p\n", zero);

    DdNode *one = DD_ONE(dd);
    printf("ONE POINTER: %p\n", one);

    do {

        if (Cudd_IsConstant(node)) {
            printf("CONST node %p: %s\n",
                node,
                node == Cudd_ReadOne(dd) ? "1" : "0");
            continue;
        }

        int index = Cudd_NodeReadIndex(node);

        // Primary input
        Abc_Obj_t *pObj = Abc_NtkPi(pNtk, index);
        const char *varName = pObj ? Abc_ObjName(pObj) : "UNKNOWN";


        // TODO: Check if the node is complemented to determine which varName is complemented
        NodeVarEntry_t *entry = NULL;
        if (!st__lookup(nodeMap, (char *)node, (char **)&entry)) {
            entry = ABC_ALLOC(NodeVarEntry_t, 1);
            entry->node = node;
            entry->varName = varName;
            entry->nodeIndex = nodeCounter;
            st__insert(nodeMap, (char *)node, (char *)entry);
        } /*else {
            printf("Existing node index: %d\n", entry->nodeIndex);
        }*/

        DdNode *complemented = Cudd_Not(node);
        NodeVarEntry_t *compEntry = NULL;
        if (!st__lookup(nodeMap, (char *)complemented, (char **)&compEntry)) {
            compEntry = ABC_ALLOC(NodeVarEntry_t, 1);
            compEntry->node = complemented;
            int len = strlen(varName) + 2; // 1 for '!', 1 for '\0'
            char *negatedVarName = ABC_ALLOC(char, len);
            snprintf(negatedVarName, len, "!%s", varName); 
            compEntry->varName = negatedVarName;
            compEntry->nodeIndex = nodeCounter + 1;
            st__insert(nodeMap, (char *)complemented, (char *)compEntry);
            // printf("Assigning new node index for complemented: %d\n", nodeCounter);=
        }

        // Add constant 1 to nodeMap
        if (!st__lookup(nodeMap, (char *)one, (char **)&entry)) {
            entry = ABC_ALLOC(NodeVarEntry_t, 1);
            entry->node = one;
            entry->varName = "1";
            entry->nodeIndex = 1;
            st__insert(nodeMap, (char *)one, (char *)entry);
        }

        // Add constant 0 to nodeMap
        if (!st__lookup(nodeMap, (char *)zero, (char **)&entry)) {
            entry = ABC_ALLOC(NodeVarEntry_t, 1);
            entry->node = zero;
            entry->varName = "0";
            entry->nodeIndex = 0;
            st__insert(nodeMap, (char *)zero, (char *)entry);
        }

        nodeCounter += 10;

        DdNode *T = Cudd_T(node);
        DdNode *E = Cudd_E(node);

        // Normalize the pointers to remove complement edges
        DdNode *T_clean = T; // Cudd_Regular(T);
        DdNode *E_clean = E; // Cudd_Regular(E);

        // // For the T child only
        // Vec_Ptr_t *inputList = NULL;
        // if (!st__lookup(nodeInputMap, (char *)T_clean, (char **)&inputList)) {
        //     inputList = Vec_PtrAlloc(16);
        //     st__insert(nodeInputMap, (char *)T_clean, (char *)inputList);
        // }

        // Then branch (always include)
        Vec_Ptr_t *thenList = NULL;
        if (!st__lookup(nodeInputMap, (char *)T_clean, (char **)&thenList)) {
            thenList = Vec_PtrAlloc(16);
            st__insert(nodeInputMap, (char *)T_clean, (char *)thenList);
        }
        InputEdge_t *thenEdge = ABC_ALLOC(InputEdge_t, 1);
        thenEdge->input = node;
        thenEdge->isTBranch = true;
        Vec_PtrPush(thenList, thenEdge);

        // Else branch (only include if not logic zero)
        if (E_clean != Cudd_ReadLogicZero(dd)) {
            Vec_Ptr_t *elseList = NULL;
            if (!st__lookup(nodeInputMap, (char *)E_clean, (char **)&elseList)) {
                elseList = Vec_PtrAlloc(16);
                st__insert(nodeInputMap, (char *)E_clean, (char *)elseList);
            }
            InputEdge_t *elseEdge = ABC_ALLOC(InputEdge_t, 1);
            elseEdge->input = node;
            elseEdge->isTBranch = false;
            Vec_PtrPush(elseList, elseEdge);
        }

        // Add the current node as a parent of the T branch
        // Vec_PtrPushUnique(inputList, node);
        
        // Print debug info
        printf("%s -> Node %p, int %d: index = %d, T = %p%s, E = %p%s\n",
            varName,
            node,
            entry->nodeIndex,
            Cudd_NodeReadIndex(node),
            T,
            Cudd_IsComplement(T) ? " (T is complemented)" : "",
            E,
            Cudd_IsComplement(E) ? " (E is complemented)" : "");

        // Add mapping for pointer to input name
        int found = 0;
        for (int i = 0; i < Vec_PtrSize(vNodeVarMap); i++) {
            NodeVarEntry_t *entry = (NodeVarEntry_t *)Vec_PtrEntry(vNodeVarMap, i);
            if (entry->node == node) {
                found = 1;
                break;
            }
        }

        if (!found) {
            NodeVarEntry_t *newEntry = ABC_ALLOC(NodeVarEntry_t, 1);
            newEntry->node = node;
            newEntry->varName = varName;
            Vec_PtrPush(vNodeVarMap, newEntry);
        }

    } while (Cudd_NextNode(gen, &node));


    printf("\n=== Node Map ===\n");
    st__generator *genSt;
    const char *key;
    char *value;

    st__foreach_item(nodeMap, genSt, &key, &value) {
        NodeVarEntry_t *entry = (NodeVarEntry_t *)value;
        printf("Node %p (%s) => index %d\n", entry->node, entry->varName, entry->nodeIndex);
    }

    // printf("\n=== Node Input Map ===\n");
    // st__generator *genInput;
    // st__foreach_item(nodeInputMap, genInput, &key, &value) {
    //     DdNode *parent = (DdNode *)key;
    //     Vec_Ptr_t *inputs = (Vec_Ptr_t *)value;
    
    //     printf("%p: ", (void *)parent);
    
    //     for (int i = 0; i < Vec_PtrSize(inputs); i++) {
    //         DdNode *child = (DdNode *)Vec_PtrEntry(inputs, i);
    //         printf("%p", (void *)child);
    
    //         if (i < Vec_PtrSize(inputs) - 1)
    //             printf(", ");
    //     }
    
    //     printf("\n");
    // }

    printf("\n=== Node Input Map ===\n");
    st__generator *genInput;
    st__foreach_item(nodeInputMap, genInput, &key, &value) {
        DdNode *parent = (DdNode *)key;
        Vec_Ptr_t *inputs = (Vec_Ptr_t *)value;
    
        printf("%p: ", parent);
    
        for (int i = 0; i < Vec_PtrSize(inputs); i++) {
            InputEdge_t *edge = (InputEdge_t *)Vec_PtrEntry(inputs, i);
            printf("%p%s", edge->input, edge->isTBranch ? " [T]" : " [E]");
            if (i < Vec_PtrSize(inputs) - 1)
                printf(", ");
        }
        printf("\n");
    }


    printf("\n\n");

    Vec_Ptr_t *path = Vec_PtrAlloc(32);
    PrintPointerChainsFromLeaf(nodeInputMap, one, path, true);
    Vec_PtrFree(path);

    // printf("Node-to-Variable Map:\n");
    // for (int i = 0; i < Vec_PtrSize(vNodeVarMap); i++) {
    //     NodeVarEntry_t *entry = (NodeVarEntry_t *)Vec_PtrEntry(vNodeVarMap, i);
    //     printf("%p -> %s\n", entry->node, entry->varName);
    // }

    // for (int i = 0; i < Vec_PtrSize(vNodeVarMap); i++) {
    //     void *pEntry = Vec_PtrEntry(vNodeVarMap, i);
    //     ABC_FREE(pEntry);
    // }
    // Vec_PtrFree(vNodeVarMap);

    // Clean up the generator
    Cudd_GenFree(gen);
}


int Bdd2Prs(Abc_Ntk_t * pNtk, int fReorder) {
    printf("Hello from bdd2prs!\n");
    
    DdManager * dd;
    Vec_Ptr_t * vFuncsGlob;
    Abc_Obj_t * pObj;
    DdNode * bFunc;
    int i;

    Abc_Ntk_t * pTemp = Abc_NtkIsStrash(pNtk) ? pNtk : Abc_NtkStrash(pNtk, 0, 0, 0);

    assert( Abc_NtkIsStrash(pTemp) );
    dd = (DdManager *)Abc_NtkBuildGlobalBdds( pTemp, 10000000, 1, fReorder, 0, 0 );
    if ( dd == NULL )
    {
        printf( "Construction of global BDDs has failed.\n" );
        return 1;
    }


    // Collect global BDDs of all primary outputs
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pTemp) );
    Abc_NtkForEachCo( pTemp, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );

    // Iterate through each BDD and print it
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i ) {
        printf("=== BDD for output %d ===\n", i);
        TraverseWithCudd(dd, bFunc, pTemp, i);
    }

    Abc_NtkFreeGlobalBdds( pNtk, 0 );
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
    Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncsGlob );
    Abc_NtkCleanCopy( pNtk );
    return 0;
}