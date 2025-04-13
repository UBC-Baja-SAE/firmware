/**
 * @file can_bridge.h
 * @brief This file provides the interface for the Kotlin dashboard to access
 *  low-level CAN bus methods.
 */

#ifndef CAN_BRIDGE_H
#define CAN_BRIDGE_H

#include <jni.h>
#include <stdint.h>

extern int categories;

/**
 * @brief Temporary data mapping a CAN message to its internal data, where
 * `data_map[i]` is the data contained in a CAN message `msg` such that
 * `msg.id % 16 == i`.
 */
extern uint64_t data_map[8];

extern "C" {    
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_get(
        JNIEnv *env,
        jobject obj,
        jint id
    );

    JNIEXPORT void JNICALL Java_org_baja_dashboard_model_DataRepository_start(
        JNIEnv *env,
        jobject obj
    );
}

#endif // CAN_BRIDGE_H