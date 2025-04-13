#include <jni.h>
#include <string.h>
#include "can_message.h"
#include "can_bridge.h"

int categories = sizeof(data_map) / sizeof(uint64_t);

uint64_t data_map[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_get(
    JNIEnv *env,
    jobject obj,
    jint id
) {
    int mod_id = id % categories;
    uint64_t data = data_map[mod_id];

    double result;
    memcpy(&result, &data, sizeof(data));
    return (jdouble) result;
}
