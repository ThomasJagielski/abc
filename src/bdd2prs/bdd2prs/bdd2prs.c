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

    int nodeCounter = 1;
    st_table *nodeMap = st_init_table(st_ptrcmp, st_ptrhash);

    // Ensure the function is in regular form (remove complement edge if any)
    node = Cudd_Regular(f);

    // Create a generator to iterate over nodes
    if (!(gen = Cudd_FirstNode(dd, node, &node))) {
        printf("Cudd_FirstNode failed.\n");
        return;
    }

    printf("Traversing BDD nodes:\n");

    do {
        char *nodeName = get_or_assign_name(nodeMap, node, &nodeCounter);

        if (Cudd_IsConstant(node)) {
            printf("CONST node [%s] %p: %s\n",
                nodeName,
                node,
                node == Cudd_ReadOne(dd) ? "1" : "0");
            continue;
        }

        int index = Cudd_NodeReadIndex(node);

        // Get the primary input object from ABC
        Abc_Obj_t *pObj = Abc_NtkPi(pNtk, index);
        const char *varName = pObj ? Abc_ObjName(pObj) : "UNKNOWN";


        DdNode *T = Cudd_T(node);
        DdNode *E = Cudd_E(node);

        char *tName = get_or_assign_name(nodeMap, T, &nodeCounter);
        char *eName = get_or_assign_name(nodeMap, Cudd_Regular(E), &nodeCounter);

        // Print debug info
        printf("%s -> Node [%s] %p: index = %d, T = [%s] %p, E = [%s] %p%s\n",
            varName,
            nodeName,
            node,
            Cudd_NodeReadIndex(node),
            tName,
            T,
            eName,
            E,
            Cudd_IsComplement(E) ? " (E is complemented)" : "");

        // printf("M%d %s %s GND GND NMOS L=0.18u W=1u ; index %d\n",
        //     nodeCounter-1, nodeName, tName, Cudd_NodeReadIndex(node));
        
    } while (Cudd_NextNode(gen, &node));

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