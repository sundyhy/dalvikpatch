
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <errno.h>

#include "log.h"
#include "mapinfo.h"
#include "dalvikpatch.h"


namespace DhDalvikPatch {

#define SYSTEM_PAGE_SIZE        4096
#define LINEAR_ALLOC_SIZE       32*1024*1024
#define LINEAR_ALLOC_NAME       "dalvik-LinearAlloc2"

#define SEARCH_MEMORY_START     1024*32
#define SEARCH_MEMORY_LEN       1024*256
#define SEARCH_TARGET_5M        5*1024*1024
#define SEARCH_TARGET_6M        6*1024*1024

DalvikPatch* sInstance = NULL;
int sRuntime = -1;


DalvikPatch::DalvikPatch()
    : m_pLinearAllocHdr(NULL)
    , m_funcAshmemCreate(NULL)
{
    MapLine* mapLine = MapInfo::firstMapByName("LinearAlloc");
    if (mapLine != NULL && mapLine->start) {
        void* dvmGlobalsPtr = findDvmGlobalsPtr();
        if (dvmGlobalsPtr != NULL) {
            //从gDvm+0*4 = gDvm+0字节处开始搜索
            //扫描vmList后面的4096个字节，每个都假设是LinearAllocHdr指针，并进行验证
            m_pLinearAllocHdr = (LinearAllocHdr*)findLinearAllocHdr(dvmGlobalsPtr + 700, 1348, mapLine->start, true);
        } else {
            LOGE("Find dvmGlobalsPtr *FAILED*, search %p-%p!", SEARCH_MEMORY_START, SEARCH_MEMORY_START + SEARCH_MEMORY_LEN);
        }

        if (m_pLinearAllocHdr != NULL) {
            m_funcAshmemCreate = (FuncAshmemCreate)findAshemeFunc();
        } else {
            LOGW("lahdr not found!!");
        }
    } else {
        LOGE("Read mapAddr from /proc/${pid}/maps failed!");
    }
}

DalvikPatch::~DalvikPatch()
{
}

DalvikPatch& DalvikPatch::instance()
{
    if (sInstance == NULL) {
        sInstance = new DalvikPatch();
    }

    return *sInstance;
}

int DalvikPatch::getMapLength()
{
    if (m_pLinearAllocHdr == NULL) {
        return 0;
    }

    return m_pLinearAllocHdr->mapLength;
}

void DalvikPatch::fixLinearAllocSize()
{
    LinearAllocHdr* pHdr = m_pLinearAllocHdr;
    if (pHdr == NULL)  {
        LOGE("Find linearAllocHdr failed!");

        return;
    }

    if (LINEAR_ALLOC_SIZE <= pHdr->mapLength) {
        LOGE("LinearAllocHdr.mapLength >= %d, don't patch dalvik!", LINEAR_ALLOC_SIZE);

        return;
    }

    int fd = ashmemCreateRegion(LINEAR_ALLOC_NAME, LINEAR_ALLOC_SIZE);
    if (fd < 0) {
        LOGE("Alloc memory from ashmem failed! %s", strerror(errno));

        return;
    }

    void* newMapAddr = mmap(NULL, LINEAR_ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (newMapAddr == MAP_FAILED) {
        LOGE("Alloc %d bytes memory by mmap failed: %s!", LINEAR_ALLOC_SIZE, strerror(errno));
        close(fd);

        return;
    }

    close(fd);
    if (mprotect(pHdr->mapAddr, pHdr->mapLength, PROT_READ | PROT_WRITE) != 0) {
        LOGE("Change old linear-alloc PROT_READ | PROT_WRITE failed: %s", strerror(errno));
        munmap(newMapAddr, LINEAR_ALLOC_SIZE);

        return;
    }

    pthread_mutex_lock(&pHdr->lock);

    memcpy(newMapAddr,  pHdr->mapAddr,  pHdr->mapLength);

    pHdr->mapAddr = (char*)newMapAddr;
    pHdr->mapLength = LINEAR_ALLOC_SIZE;

    if (mprotect(pHdr->mapAddr, SYSTEM_PAGE_SIZE, PROT_NONE) != 0) {
        pthread_mutex_unlock(&pHdr->lock);
        LOGE("Change new linear-alloc first page to PORT_NONE failed!: %s", strerror(errno));

        return;
    }

    pthread_mutex_unlock(&pHdr->lock);

    LOGE("Patch dalvik-LinearAlloc success!!!");

    return;
}

void* DalvikPatch::findAshemeFunc()
{
    void* hdr = dlopen("libcutils.so", RTLD_NOW);
    if (hdr == NULL ) {
        return NULL;
    }

    void* pFunc = dlsym(hdr, "ashmem_create_region");
    dlclose(hdr);

    return pFunc;
}

void* DalvikPatch::findDvmGlobalsPtr() {
    void* dvmGlobalsPtr = NULL;
    void* hdr = dlopen("libdvm.so", RTLD_NOW);
    if (hdr == NULL) {
        LOGE("Open libdvm.so *FAILED*!");

        return NULL;
    }

    dvmGlobalsPtr = dlsym(hdr, "gDvm");
    dlclose(hdr);

    if (dvmGlobalsPtr == NULL) {
        LOGE("Find symbol 'gDvm' *FAILED*!");

        return NULL;
    }


    return dvmGlobalsPtr;
}

void* DalvikPatch::findLinearAllocHdr(const void* searchStart, uint32_t searchLen, const void* mapAddr, bool asPointer)
{
    uint32_t* pScanAddr = (uint32_t*)searchStart;

    //treate every 4byte from searchStart to searchStart+searchLen as a LinearAllocHdr pointer?
    if (asPointer) {
        for (int i = 0; i < searchLen/4; i++) {
            pScanAddr++;
            if (!MapInfo::isReadable((uint8_t*)pScanAddr, 4)) {
                continue;
            }

            if (!MapInfo::isReadable((uint8_t*)*pScanAddr, sizeof(LinearAllocHdr)) ) {
                continue;
            }

            LinearAllocHdr* pLinearAllocHdr = (LinearAllocHdr*)*pScanAddr;
            if ((void*)pLinearAllocHdr->mapAddr == mapAddr) {
                return pLinearAllocHdr;
            }
        }
    } else {
        for (int i = 0; i < searchLen/4; i++) {
            pScanAddr++;
            if (!MapInfo::isReadable((uint8_t*)pScanAddr, 16)) {
                continue;
            }

            int mapLength = *(pScanAddr + 1);
            if (mapAddr == (const void*)*pScanAddr && (mapLength == SEARCH_TARGET_5M
                                                       || mapLength == SEARCH_TARGET_6M)) {
                LinearAllocHdr* pLinearAllocHdr = (LinearAllocHdr*)(pScanAddr - 2);
                return pLinearAllocHdr;
            }
        }
    }

    return NULL;
}

int DalvikPatch::ashmemCreateRegion(const char* name, size_t size) {
    if (m_funcAshmemCreate == NULL) {
        LOGE("Find function ashmem_create_region failed!");
        return -1;
    }

    return m_funcAshmemCreate(name, size);
}

bool is_runtime_dalvik() {
    if (sRuntime < 0) {
        if (MapInfo::findMapByName("libdvm.so") != NULL) {
            sRuntime = 0;
        } else if (MapInfo::findMapByName("libart.so") != NULL) {
            sRuntime = 1;
        } else {
            sRuntime = 2;
        }
    }

    return sRuntime == 0;
}

}
