#ifndef __DHDALVIKPATCH_DALVIKPATCH_H__
#define __DHDALVIKPATCH_DALVIKPATCH_H__

#include <pthread.h>

namespace DhDalvikPatch {

typedef struct LinearAllocHdr {
    int     curOffset;          /* offset where next data goes */
    pthread_mutex_t lock;       /* controls updates to this struct */

    char*   mapAddr;            /* start of mmap()ed region */
    int     mapLength;          /* length of region */
    int     firstOffset;        /* for chasing through */

    short*  writeRefCount;      /* for ENFORCE_READ_ONLY */
} LinearAllocHdr;


typedef int (*FuncAshmemCreate)(const char *name, size_t size);


class DalvikPatch {
public:
    void*   getMapAddr();
    int     getMapLength();
    void    fixLinearAllocSize();

    static DalvikPatch& instance();

private:
    DalvikPatch();
    ~DalvikPatch();

    void*   findAshemeFunc();
    void*   findDvmGlobalsPtr();
    void*   findLinearAllocHdr(const void* searchStart, uint32_t searchLen, const void* mapAddr, bool asPointer);
    int     ashmemCreateRegion(const char* name, size_t size);

    LinearAllocHdr*     m_pLinearAllocHdr;
    FuncAshmemCreate    m_funcAshmemCreate;
};

bool is_runtime_dalvik();

}

#endif
