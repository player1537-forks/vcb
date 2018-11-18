
#include <iostream>
using namespace std;

#include "hashtable.h"
#include "MemRoot.h"
#include <string.h>
#include <stdlib.h>

MemRoot::MemRoot(size_t size)
{

    pList = (MemEnt *) 0;
    pTrash = (MemEnt *) 0;
    iLogSize = MEMSTART;
    iNumLeft = 0;
    iSize = align(size);
}
MemRoot::~MemRoot()
{

    MemEnt *temp;

    if ((temp = pList))
        while (pList) {
            temp = pList->next;
            free((char *) pList);
            pList = temp;
        }

}
void *MemRoot::NewX()
{
    //cout<<"renewx"<<endl;

    MemEnt *temp;

    if (pTrash) {               /* get a node from the trash heap */
        temp = pTrash;
        pTrash = temp->next;
        memset(temp, 0, iSize);
    } else {
        iNumLeft = iSize * ((ub4) 1 << iLogSize);
        if (iNumLeft < MEMMAX)
            iLogSize++;
        temp = (MemEnt *) remalloc(sizeof(MemEnt) + iNumLeft);
        temp->next = pList;
        pList = temp;
        iNumLeft -= iSize;
        temp = (MemEnt *) ((char *) (pList + 1) + iNumLeft);
    }
    return (void *) temp;
}
void *MemRoot::remalloc(int len)
{
    void *x = malloc(len);

    if (!x) {
        cout << "MALLOC ERROR" << endl;
        exit(1);
    }
    memset(x, 0, len);

    return x;

}
void *MemRoot::NewMem()
{
//  cout<<"renew"<<endl;
    return iNumLeft ? ((char *) (pList + 1)) + (iNumLeft -= iSize) : NewX();

}
void MemRoot::DelMem(MemEnt * item)
{
    item->next = pTrash;
    pTrash = item;
}
