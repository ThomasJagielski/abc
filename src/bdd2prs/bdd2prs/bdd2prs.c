// src/prs/bdd2prs.c

#include <stdio.h>
#include "base/abc/abc.h"
#include "bdd/cudd/cudd.h"
#include "bdd/cudd/cuddInt.h"


#define st_table st__table
#define st_init_table st__init_table
#define st_free_table st__free_table
#define st_ptrcmp st__ptrcmp
#define st_ptrhash st__ptrhash
#define st_insert st__insert
#define st_lookup st__lookup 

char* get_or_assign_name(st_table *nodeMap, DdNode *n, int *nodeCounter) {
    char *name;
    if (!st_lookup(nodeMap, (char *)n, (char **)&name)) {
        name = (char *)malloc(20);

        if (Cudd_IsConstant(n)) {
            sprintf(name, "t%d", (*nodeCounter)++);  // constants prefixed with 't'
        } else {
            sprintf(name, "n%d", (*nodeCounter)++);  // regular nodes prefixed with 'n'
        }

        st_insert(nodeMap, (char *)n, name);
    }
    return name;
}

void TraverseWithCudd(DdManager *dd, DdNode *f, Abc_Ntk_t *pNtk) {
    DdGen *gen;
    DdNode *node;
    DdNode *lastNode;
    char prsLine[250];
    prsLine[0] = '\0'; // Initialize to empty string
    int condition = 0;
    

    int nodeCounter = 1;
    int i=0;
    st_table *nodeMap = st_init_table(st_ptrcmp, st_ptrhash);

    // Ensure the function is in regular form (remove complement edge if any)
    node = Cudd_Regular(f);

    // Create a generator to iterate over nodes
    if (!(gen = Cudd_FirstNode(dd, node, &node))) {
        printf("Cudd_FirstNode failed.\n");
        return;
    }

    printf("Traversing BDD nodes:\n");       

    DdNode *zero = Cudd_ReadLogicZero(dd);
    printf("Constant ZERO node pointer: %p\n", zero);

    DdNode *one = DD_ONE(dd);
    printf("ONE_POINTER: %p\n", one);

    // printf("Complemented 1: %d\n", Cudd_IsComplement(one));

    // Cudd_PrintDebug(dd, one, 0, 3); // depth 0, verbosity 2

    // Cudd_PrintDebug(dd, zero, 0, 3); // depth 0, verbosity 2

    if (Cudd_Regular(zero) == Cudd_ReadOne(dd) && Cudd_IsComplement(zero)) {
        printf("This is actually LOGICAL ZERO\n");
    }


    DdNode *reg = Cudd_Regular(zero);
    if (reg == one) {
        if (Cudd_IsComplement(zero)) {
            printf("Node represents logic 0\n");
        } else {
            printf("Node represents logic 1\n");
        }
    }



    // DdNodePtr nodelist = dd->constants.nodelist;

    do {
        char *nodeName = get_or_assign_name(nodeMap, node, &nodeCounter);

        if (Cudd_IsConstant(node)) {
            printf("CONST node [%s] %p: %s\n",
                nodeName,
                node,
                node == Cudd_ReadOne(dd) ? "1" : "0");
            continue;
        }

        // if (Cudd_IsConstant(Cudd_Regular(node))) {
        //     DdNode *one = Cudd_ReadOne(dd);
        //     const char *value;
        
        //     if (node == one) {
        //         value = "1";
        //     } else if (node == Cudd_Not(one)) {
        //         value = "0";
        //     } else {
        //         value = "UNKNOWN_CONST";
        //     }
        
        //     printf("CONST node [%s] %p: %s\n", nodeName, node, value);
        //     continue;
        // }

        int index = Cudd_NodeReadIndex(node);

        // Primary input
        Abc_Obj_t *pObj = Abc_NtkPi(pNtk, index);
        const char *varName = pObj ? Abc_ObjName(pObj) : "UNKNOWN";

        DdNode *T = Cudd_T(node);
        DdNode *E = Cudd_E(node);

        char *tName = get_or_assign_name(nodeMap, T, &nodeCounter);
        char *eName = get_or_assign_name(nodeMap, Cudd_Regular(E), &nodeCounter);

        // Print debug info
        printf("%s -> Node [%s] %p: index = %d, T = [%s] %p%s, E = [%s] %p%s\n",
            varName,
            nodeName,
            node,
            Cudd_NodeReadIndex(node),
            tName,
            T,
            Cudd_IsComplement(T) ? " (T is complemented)" : "",
            eName,
            E,
            Cudd_IsComplement(E) ? " (E is complemented)" : "");

        // if (i >= 1) {
        //     DdNode *lastT = Cudd_T(lastNode);
        //     DdNode *lastE = Cudd_E(lastNode);
        //     printf("Last Node: %p: index = %d, T = %p, E = %p%s\n",
        //         lastNode,
        //         Cudd_NodeReadIndex(lastNode),
        //         lastT,
        //         lastE,
        //         Cudd_IsComplement(lastE) ? " (E is complemented)" : "");
        // }

        if ( i >= 1 ) {
            // TODO: Check if a node is a complement
            char *lastNodeName = get_or_assign_name(nodeMap, lastNode, &nodeCounter-1);
            DdNode *lastT = Cudd_T(lastNode);
            DdNode *lastE = Cudd_E(lastNode);
            int lastIndex = Cudd_NodeReadIndex(lastNode);

            Abc_Obj_t *pObj = Abc_NtkPi(pNtk, lastIndex);
            const char *lastVarName = pObj ? Abc_ObjName(pObj) : "UNKNOWN";

            // New Condition
            if ( lastT == Cudd_ReadOne(dd) ) {
                // strcpy(prsLine, "");
                strncat(prsLine, "(", sizeof(prsLine) - strlen(prsLine) - 1);
                strncat(prsLine, lastVarName, sizeof(prsLine) - strlen(prsLine) - 1);
            }

            // last node of leaf
            if ( T == Cudd_ReadOne(dd) ) {
                if (i > 1) {
                    strncat(prsLine, ") -> ", sizeof(prsLine) - strlen(prsLine) - 1);
                    if (condition == 1) {
                        strncat(prsLine, lastNodeName, sizeof(prsLine) - strlen(prsLine) - 1);
                    } else if (condition == 2) {
                        strncat(prsLine, nodeName, sizeof(prsLine) - strlen(prsLine) - 1);
                    }
                    strncat(prsLine, ";\n", sizeof(prsLine) - strlen(prsLine) - 1);
                    // printf("%s", prsLine);
                    condition = 0;
                }
            }

            // AND condition
            if ( (T == lastNode) && (E == lastE) ) {
                strncat(prsLine, " ^ ", sizeof(prsLine) - strlen(prsLine) - 1);
                strncat(prsLine, varName, sizeof(prsLine) - strlen(prsLine) - 1);
                condition = 1;
            }

            // OR condition
            if ( (T == lastT) && (lastNode == E) ) {
                strncat(prsLine, " | ", sizeof(prsLine) - strlen(prsLine) - 1);
                strncat(prsLine, varName, sizeof(prsLine) - strlen(prsLine) - 1);
                condition = 2;
            }

            // XOR condition
            // if ( (T == lastT) && ((lastNode == E) || (lastE == node)) ) {
            //     strncat(prsLine, " | ", sizeof(prsLine) - strlen(prsLine) - 1);
            //     strncat(prsLine, varName, sizeof(prsLine) - strlen(prsLine) - 1);
            //     condition = 3;
            // }

        }
        
        lastNode = node;
        i = i + 1;
    } while (Cudd_NextNode(gen, &node));

    strncat(prsLine, ") -> ", sizeof(prsLine) - strlen(prsLine) - 1);
    char *nodeName = get_or_assign_name(nodeMap, node, &nodeCounter);
    if (condition == 2) {
        DdNode *E = Cudd_E(node);
        char *eName = get_or_assign_name(nodeMap, Cudd_Regular(E), &nodeCounter);
        strncat(prsLine, eName, sizeof(prsLine) - strlen(prsLine) - 1);
    } else {
        strncat(prsLine, nodeName, sizeof(prsLine) - strlen(prsLine) - 1);
    }
    strncat(prsLine, ";\n", sizeof(prsLine) - strlen(prsLine) - 1);

    // Print out the last node condition if there isn't a new condition
    printf("%s", prsLine);

    // Clean up the generator
    Cudd_GenFree(gen);
    st_free_table(nodeMap);
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
    // // --- NOTE: ADDED THIS!
    // Cudd_PrintDebug( dd, bFunc, Abc_NtkCiNum(pNtk), 3 );  // verbose level 2
    // // --- 

    // Collect global BDDs of all primary outputs
    vFuncsGlob = Vec_PtrAlloc( Abc_NtkCoNum(pTemp) );
    Abc_NtkForEachCo( pTemp, pObj, i )
        Vec_PtrPush( vFuncsGlob, Abc_ObjGlobalBdd(pObj) );

    // Iterate through each BDD and print it
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i ) {
        printf("=== BDD for output %d ===\n", i);
        // Cudd_PrintDebug( dd, bFunc, Abc_NtkCiNum(pTemp), 3 );
        TraverseWithCudd(dd, bFunc, pTemp);
    }

    // cleanup
    // if ( pTemp != pNtk )
    //     Abc_NtkDelete( pTemp );

    Abc_NtkFreeGlobalBdds( pNtk, 0 );
    Vec_PtrForEachEntry( DdNode *, vFuncsGlob, bFunc, i )
    Cudd_RecursiveDeref( dd, bFunc );
    Vec_PtrFree( vFuncsGlob );
    // Extra_StopManager( dd );
    Abc_NtkCleanCopy( pNtk );
    return 0;
}