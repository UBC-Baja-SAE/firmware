/**
 * @file can_bridge.h
 * @brief This file provides the implementation for methods in the Kotlin
 * dashboard source code that provides an interface for interacting with CAN bus
 * data. The methods implemented by this file are contained in 
 * `src/main/kotlin/model/DataRepository`.
 */

#ifndef CAN_BRIDGE_H
#define CAN_BRIDGE_H

#include <jni.h>

/**
 * @brief This is the prefix for the Kotlin methods that are implemented by this
 * class. This macro is the required prefix for a given JNI method in the JVM.
 * See [the JNI documentation](https://docs.oracle.com/en/java/javase/17/docs/specs/jni/design.html)
 * for more information.
 */
#define REPOSITORY_CLASS_PREFIX Java_org_baja_dashboard_model_DataRepository_

/**
 * @brief Concatenates two macros together such that there are no spaces.
 */
#define JOIN(a, b) a ## b

/**
 * @brief Expands `a` and `b` if they are macros, and concatenates them.
 */
#define EXPAND_JOIN(a, b) JOIN(a, b)

/**
 * @brief A macro that given a method name `method`, it returns the name of the
 * method appended to the `REPOSITORY_CLASS_PREFIX`, such that it is a valid
 * method in the Kotlin interface that this implements.
 */
#define REPOSITORY_METHOD_NAME(name) EXPAND_JOIN(REPOSITORY_CLASS_PREFIX, name)

/**
 * @brief A macro that generates the code to implement JVM methods using JNI.
 * This returns a method declaration such that it returns `return_type`, and
 * that it implements a JVM method named `method_name`.
 * 
 * Suppose there is some Kotlin method in the package `org.baja.dashboard`:
 * ```
 *  // src/main/kotlin/model/DataRepository.kt
 *  class DataRepository {
 *      external fun example(): void
 * 
 *      // ...
 *  }
 * ```
 * Then, the implementation in C/C++ must be:
 * ```
 *  JNIEXPORT void JNICALL Java_org_baja_dashboard_model_DataRepository_example(JNIEnv *env, jobject obj)
 *  {
 *      // ...
 *  }
 * ```
 * This can be replaced by this macro as:
 * ```
 *  REPOSITORY_METHOD(void, example)
 *  {
 *      // ...
 *  }
 * ```
 * Another preprocessor macro can be used to further shorten this to:
 * ```
 *  #define anyAlias REPOSITORY_METHOD(void, example)
 * 
 *  anyAlias
 *  {
 *      // ...
 *  }
 * ```
 * 
 * Note that these macros are primarily used because they're cool.
 * 
 * @param return_type   the type of the method's return value, where the type
 * defined in the `<jni.h>` header corresponds directly to Java types. See
 * [the JNI documentation](https://docs.oracle.com/en/java/javase/17/docs/specs/jni/design.html)
 * for more information.
 * @param method_name   the name of the method. This must \a exactly match the
 * name of the method in the `DataRepository` class in Kotlin.
 */
#define REPOSITORY_METHOD(return_type, method_name) \
    JNIEXPORT return_type JNICALL \
    REPOSITORY_METHOD_NAME(method_name)(JNIEnv *env, jobject obj)

/**
 * @brief Retrieves the last observed speed of the vehicle.
 * @return the speed as a 64-bit double-precision floating point number.
 */
#define getSpeed        REPOSITORY_METHOD(jdouble, getSpeed)

/**
 * @brief Retrieves the last observed engine temperature of the vehicle.
 * @return the temperature as a 64-bit double-precision floating point number.
 */
#define getTemperature  REPOSITORY_METHOD(jdouble, getTemperature)

/**
 * @brief Retrieves the last observed revolutions per minute (RPM) of the
 * vehicle's crankshaft.
 * @return the RPM as a 64-bit double-precision floating point number.
 */
#define getRPM          REPOSITORY_METHOD(jdouble, getRPM)

/**
 * @brief Retrieves the last observed fuel level of the vehicle.
 * @return the fuel levels as a 64-bit double-precision floating point number.
 */
#define getFuel         REPOSITORY_METHOD(jdouble, getFuel)

/**
 * @brief Launches the CAN processing application, enabling the dashboard
 * application to receive, process, and send CAN bus data.
 */
#define startProcessor  REPOSITORY_METHOD(void, start)

/*
 * Use `extern "C"` to prevent name mangling on compilation, otherwise it will
 * not match its respective Kotlin method.
 * Check the post-compilation method names with:
 *  `nm [shared library file] | grep -i repository`
 */
extern "C" {
    getSpeed;

    getTemperature;

    getRPM;

    getFuel;

    startProcessor;
}

#endif // CAN_BRIDGE_H