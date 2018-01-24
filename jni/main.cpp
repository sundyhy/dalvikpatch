#include <jni.h>
#include "log.h"
#include "dalvikpatch.h"

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_sundyhy_mobile_dalvikpatch_DalvikPatch_isDalvik(JNIEnv *pJniEnv, jclass) {
    return (jboolean)DhDalvikPatch::is_runtime_dalvik();
}

JNIEXPORT jint JNICALL Java_com_sundyhy_mobile_dalvikpatch_DalvikPatch_getMapLength(JNIEnv *pJniEnv, jclass) {
    if (DhDalvikPatch::is_runtime_dalvik()) {
        return DhDalvikPatch::DalvikPatch::instance().getMapLength();
    }

    return 0;
}

JNIEXPORT void JNICALL Java_com_sundyhy_mobile_dalvikpatch_DalvikPatch_adjustLinearAlloc(JNIEnv *pJniEnv, jclass) {
    if (DhDalvikPatch::is_runtime_dalvik()) {
        DhDalvikPatch::DalvikPatch::instance().fixLinearAllocSize();
    }
}

#ifdef __cplusplus
}
#endif
