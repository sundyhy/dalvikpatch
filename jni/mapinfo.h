#ifndef __DHDALVIKPATCH_MAPINFO_H__
#define __DHDALVIKPATCH_MAPINFO_H__

#include <unistd.h>

namespace DhDalvikPatch {

struct MapLine {
    struct MapLine*    next;
    uint8_t*            start;
    uint8_t*            end;
    uint32_t            limits;
    char                name[];
};

class MapInfo {
public:
    static bool isWritable(uint8_t* addr);
    static bool isWritable(uint8_t* addr, uint32_t len);

    static bool isReadable(uint8_t* addr);
    static bool isReadable(uint8_t* addr, uint32_t len);

    static MapLine* findMapByName(const char* name);
    static MapLine* firstMapByName(const char* name);
    static const char *mapAddressToName(const void* pc, const char* def,void const** start);

public:
    MapInfo();
    ~MapInfo();

private:
    MapLine* getMapInfoList();
    MapLine* parseMapsLine(char *line);

    const char* mapToName(uint8_t* pc, const char* def, uint32_t* start);

private:
    MapLine*    milist;
};

}

#endif  // __DHDALVIKPATCH_MAPINFO_H__
