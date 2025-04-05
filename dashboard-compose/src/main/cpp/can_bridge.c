#include <jni.h>
#include "can_message.h"
#include "can_bridge.h"

extern uint64_t data_map[categories] = {
    0, 
    0, 
    0,
    0,
    0, 
    0,
    0,
    0,
    0,
    0,
    0,
    0, 
    0,
    0,
    0,
    0
};

JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_get(
    JNIEnv *env,
    jobject obj,
    jint id
) {
    int mod_id = id % categories;
    uint64_t data = data_map[mod_id];
    return (jdouble) data;
}
