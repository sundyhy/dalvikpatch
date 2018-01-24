
#include <stdlib.h>
#include <stdio.h>

#include "log.h"
#include "mapinfo.h"

namespace DhDalvikPatch {

#define MEMORY_ACCESS_READ      0x01
#define MEMORY_ACCESS_WRITE     0x02
#define MEMORY_ACCESS_EXEC      0x04
#define MEMORY_ACCESS_PRIVATE   0x08

static MapInfo sMapInfo; 

MapInfo::MapInfo()
    : milist(0)
{
}

MapInfo::~MapInfo()
{
    while (milist) {
        MapLine *next = milist->next;
        free(milist);
        milist = next;
    }
}

bool MapInfo::isReadable(uint8_t* addr)
{
    return isReadable(addr, 1);
}

bool MapInfo::isReadable(uint8_t* addr, uint32_t len)
{
    uint8_t* curAddr = addr;
    uint8_t* lastAddr = addr + len;
    bool allReadable = true;
    MapLine* miAccess = NULL;
    MapLine* mi = sMapInfo.getMapInfoList();

    while(mi) {
        if (curAddr >= lastAddr) {
            break;
        }

        while ( (curAddr < lastAddr) && (curAddr >= mi->start) && (curAddr < mi->end) ) {
            miAccess = mi;
            curAddr++;
        }

        if (miAccess && !(miAccess->limits & MEMORY_ACCESS_READ)) {
            allReadable = false;
            break;
        }

        mi = mi->next;
    }

    return miAccess && allReadable && (curAddr >= lastAddr);
}

bool MapInfo::isWritable(uint8_t* addr)
{
    return isWritable(addr, 1);
}

bool MapInfo::isWritable(uint8_t* addr, uint32_t len)
{
    uint8_t* curAddr = addr;
    bool allWritable = true;
    MapLine* miAccess = NULL;
    MapLine* mi = sMapInfo.getMapInfoList();
    while(mi) {
        if (curAddr >= (addr + len)) {
            break;
        }

        while ( (curAddr < (addr+len)) && (curAddr >= mi->start) && (curAddr < mi->end) ) {
            miAccess = mi;
            curAddr++;
        }

        if (!(miAccess->limits & MEMORY_ACCESS_WRITE)) {
            allWritable = false;
            break;
        }

        mi = mi->next;
    }

    return miAccess && allWritable && (curAddr >= (addr + len));
}


MapLine* MapInfo::findMapByName(const char* name)
{
    MapLine* t = NULL;
    MapLine* mi = sMapInfo.getMapInfoList();
    while (mi && name) {
        if (strstr(mi->name, name) != NULL) {
            t = mi;
            break;
        }

        mi = mi->next;
    }

    return t;
}

MapLine* MapInfo::firstMapByName(const char *name)
{
    MapLine* t = NULL;
    MapLine* mi = sMapInfo.getMapInfoList();
    while (mi && name && strlen(name) > 0) {
        if (strstr(mi->name, name) != NULL) {
            t = (t == NULL) ? mi : t;
            t = (mi->start < t->start) ? mi : t;
        }
        mi = mi->next;
    }

    return t;
}

const char *MapInfo::mapAddressToName(const void* pc, const char* def, void const** start)
{
    uint32_t s;
    char const* name = sMapInfo.mapToName((uint8_t*)pc, def, &s);
    if (start) {
        *start = (void*)s;
    }

    return name;
}

const char *MapInfo::mapToName(uint8_t* pc, const char* def, uint32_t* start)
{
    MapLine* mi = getMapInfoList();
    while(mi) {
        if ((pc >= mi->start) && (pc < mi->end)) {
            if (start) {
                *start = (uint32_t)mi->start;
            }

            return mi->name;
        }

        mi = mi->next;
    }
    if (start) {
        *start = 0;
    }

    return def;
}

MapLine* MapInfo::parseMapsLine(char *line)
{
    MapLine *mi;
    int len = strlen(line);
    if (len < 1) return NULL;

    int lastIndex = len - 1;
    if (line[lastIndex] == '\n' || line[lastIndex] == '\r') {
        len             = lastIndex;
        line[lastIndex] = 0;
    }

    if (len < 50) {
        mi = (MapLine*)malloc(sizeof(MapLine) + 1);
    } else {
        mi = (MapLine*)malloc(sizeof(MapLine) + (len - 47));
    }

    if (mi == 0) return NULL;

    mi->start   = (uint8_t*)strtoull(line, 0, 16);
    mi->end     = (uint8_t*)strtoull(line + 9, 0, 16);
    mi->next    = 0;
    mi->limits  = 0;
    if (len < 50) {
        mi->name[0] = 0;
    } else {
        strcpy(mi->name, line + 49);
    }

    if (line[18] == 'r')    mi->limits = mi->limits | MEMORY_ACCESS_READ;
    if (line[19] == 'w')    mi->limits = mi->limits | MEMORY_ACCESS_WRITE;
    if (line[20] == 'x')    mi->limits = mi->limits | MEMORY_ACCESS_EXEC;
    if (line[21] == 'p')    {
        mi->limits = mi->limits | MEMORY_ACCESS_PRIVATE;
    } else {
        mi->limits = mi->limits & (~MEMORY_ACCESS_PRIVATE);
    }

    return mi;
}

MapLine* MapInfo::getMapInfoList()
{
    if (milist == 0) {
        char data[1024];
        FILE *fp;
        sprintf(data, "/proc/%d/maps", getpid());
        fp = fopen(data, "r");
        if (fp) {
            while(fgets(data, 1024, fp)) {
                MapLine *mi = parseMapsLine(data);
                if(mi) {
                    mi->next = milist;
                    milist = mi;
                }
            }
            fclose(fp);
        }
    }
    return milist;
}

}
