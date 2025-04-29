// src/prs/bdd2prs.c

#include <stdio.h>
#include <stdbool.h>
#include "base/abc/abc.h"
#include "bdd/cudd/cudd.h"
#include "bdd/cudd/cuddInt.h"
#include "bdd2prs.h"

#define st_table st__table
#define st_init_table st__init_table
#define st_free_table st__free_table
#define st_ptrcmp st__ptrcmp
#define st_ptrhash st__ptrhash
#define st_strcmp st__strcmp
#define st_strhash st__strhash
#define st_insert st__insert
#define st_lookup st__lookup 

char* get_or_assign_name(st_table *nodeMap, DdNode *n, int *nodeCounter) {
    char *name;
    if (!st_lookup(nodeMap, (char *)n, (char **)&name)) {
        name = (char *)malloc(20);

        if (Cudd_IsConstant(n)) {
            sprintf(name, "t%d", (*nodeCounter)++);  // constants prefixed with 't'
        } else {
            sprintf(name, "@n%d", (*nodeCounter)++);  // regular nodes prefixed with 'n'
        }

        st_insert(nodeMap, (char *)n, name);
    }
    return name;
}

char* get_or_assign_name_mapping(st_table *nameMap, const char *key, const char *value) {
    char *existingValue = NULL;
    if (st_lookup(nameMap, (char *)key, &existingValue)) {
        // Key exists, return the existing value
        return existingValue;
    } else {
        // Insert new key/value
        char *keyCopy = strdup(key);
        char *valueCopy = strdup(value);
        st_insert(nameMap, keyCopy, valueCopy);
        return valueCopy;
    }
}


bool is_connected_to_base(DdManager *dd, DdNode *node) {
    DdNode *zero = Cudd_ReadLogicZero(dd);
    DdNode *one = DD_ONE(dd);

    DdNode *T = Cudd_T(node);
    DdNode *E = Cudd_E(node);

    if ( (T == zero) || (E == one) ) {
        return 1;
    } else if ( (T == one) || (E == zero) ) {
        return 1;
    } else {
        return 0;
    }
}

bool is_AND(DdNode *lastNode, DdNode *node) {
    DdNode *T = Cudd_T(node);
    DdNode *E = Cudd_E(node);

    // DdNode *lastT = Cudd_T(lastNode);
    DdNode *lastE = Cudd_E(lastNode);

    if ( (T == lastNode) && (E == lastE) ) {
        return 1;
    } else {
        return 0;
    }
}

bool is_OR(DdNode *lastNode, DdNode *node) {
    DdNode *T = Cudd_T(node);
    DdNode *E = Cudd_E(node);

    DdNode *lastT = Cudd_T(lastNode);
    // DdNode *lastE = Cudd_E(lastNode);

    if ( (T == lastT) && (lastNode == E) ) {
        return 1;
    } else {
        return 0;
    }
}

void TraverseWithCudd(DdManager *dd, DdNode *f, Abc_Ntk_t *pNtk, int iPo) {
    DdGen *gen;
    DdNode *node;
    DdNode *lastNode;
    char prsLine[1024] = "";  // Make sure this is large enough
    int offset = 0;

    int nodeCounter = 1;
    int i=0;
    int or_flag=0;
    int start_flag=0;
    st_table *nodeMap = st_init_table(st_ptrcmp, st_ptrhash);
    st_table *nameMap = st_init_table(strcmp, st_strhash);

    Abc_Obj_t *pPo = Abc_NtkCo(pNtk, iPo);
    const char *outputName = Abc_ObjName(pPo);
    DdNode *outputPointer = f;

    // Add the output to the nodeMap
    st_insert(nodeMap, (char *)outputPointer, (char *)outputName);

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
        char *nodeName = get_or_assign_name(nodeMap, node, &nodeCounter);

        if (Cudd_IsConstant(node)) {
            printf("CONST node [%s] %p: %s\n",
                nodeName,
                node,
                node == Cudd_ReadOne(dd) ? "1" : "0");
            continue;
        }

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

        if ( T != one ) {
            // TODO: Include checking for if there is a complemented version or not
            // TODO: Treat everything as the start of an OR tree and then base decisions on that

            char *lastNodeName = get_or_assign_name(nodeMap, lastNode, &nodeCounter);
        

            if ( i >= 1 ) {
                if ( (i >= 1) && is_OR(lastNode, node) ) {
                    printf("%s maps to %s!!\n", eName, nodeName);
                    get_or_assign_name_mapping(nameMap, eName, nodeName);
                }
                // if ( is_OR(lastNode, node) ) {
                //     offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, " | (%s & %s)", tName, varName);
                //     printf("%s maps to %s!!\n", eName, nodeName);
                //     or_flag = 1;
                //     start_flag = 0;
                // } else if (1 == or_flag ) {
                //     offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, " -> %s", nodeName);
                //     or_flag = 0;
                //     start_flag = 0;
                // } else {
                    offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, "(%s & %s) -> %s\n", tName, varName, nodeName);
                    // if (start_flag == 1) {
                    //     offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, " -> %s\n", lastNodeName);    
                    // } else {
                    //     offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, "(%s & %s)", tName, varName);
                    //     start_flag = 1;
                    // }
                    or_flag = 0;
                }
            // }
        } else {
            offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, "%s -> %s\n", varName, nodeName);
            if ( (i >= 1) && is_OR(lastNode, node) ) {
                printf("%s maps to %s!!\n", eName, nodeName);
                get_or_assign_name_mapping(nameMap, eName, nodeName);
            }
        }
        
        lastNode = node;
        i = i + 1;
    } while (Cudd_NextNode(gen, &node));

    // Print out the last node condition if there isn't a new condition
    char *nodeName = get_or_assign_name(nodeMap, node, &nodeCounter);
    char *lastNodeName = get_or_assign_name(nodeMap, lastNode, &nodeCounter);
    if (1 == or_flag) {
        offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, " -> %s", nodeName);
        or_flag = 0;
    } else if (1 == start_flag) {
        offset += snprintf(prsLine + offset, sizeof(prsLine) - offset, " -> %s\n", lastNodeName); 
        start_flag = 0;
    }
    
    // printf("\n=== Name Mappings ===\n");
    // st__generator *gen2;
    // char *key, *value;
    // for (gen2 = st__init_gen(nameMap); st__gen(gen2, (char **)&key, (char **)&value); ) {
    //     printf("%s -> %s\n", key, value);
    // }
    // st__free_gen(gen2);
    // printf("=====================\n");
    
    
    // printf("\n\n%s\n\n", prsLine);

    //////

  // 3. Replace all names inside prsLine
char finalLine[4096] = "";
char *cursor = prsLine;
char *dest = finalLine;

while (*cursor) {
    int replaced = 0;

    st__generator *gen3;
    char *key, *value;

    // Try to match at cursor
    for (gen3 = st__init_gen(nameMap); st__gen(gen3, (char **)&key, (char **)&value); ) {
        int keyLen = strlen(key);

        if (strncmp(cursor, key, keyLen) == 0) {
            // Match found

            char tempValue[1024];
            strcpy(tempValue, value);

            int fullyResolved = 0;

            // Keep resolving the mapping if it maps to something else
            while (!fullyResolved) {
                fullyResolved = 1;
                st__generator *innerGen;
                char *innerKey, *innerValue;
                for (innerGen = st__init_gen(nameMap); st__gen(innerGen, (char **)&innerKey, (char **)&innerValue); ) {
                    if (strcmp(tempValue, innerKey) == 0) {
                        strcpy(tempValue, innerValue);
                        fullyResolved = 0; // found a new mapping, continue
                        break;
                    }
                }
                st__free_gen(innerGen);
            }

            int tempLen = strlen(tempValue);
            memcpy(dest, tempValue, tempLen);
            dest += tempLen;
            cursor += keyLen;
            replaced = 1;
            break;
        }
    }
    st__free_gen(gen3);

    if (!replaced) {
        *dest++ = *cursor++;
    }
}
*dest = '\0';


// Final output
printf("\n=== Final Replaced String ===\n");
printf("%s\n", finalLine);
printf("=============================\n");

    //////




    // Clean up the generator
    Cudd_GenFree(gen);
    st_free_table(nameMap);
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
        TraverseWithCudd(dd, bFunc, pTemp, i);

        // Abc_Obj_t *pPo = Abc_NtkCo(pTemp, i);
        // const char *outputName = Abc_ObjName(pPo);

        // printf("=== BDD for output %d: %s ===\n", i, outputName);
        // printf("Output node pointer: %p\n", bFunc);

        // TraverseWithCudd(dd, bFunc, pTemp, i);
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