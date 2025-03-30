#include <jni.h>

JNIEXPORT void JNICALL Java_org_baja_dashboard_model_DataRepository_test(
    JNIEnv *env,
    jobject obj
) {
    printf("Hello from C++\n");
    return;
}
