#include <jni.h>
#include <cstring>
#include <iostream>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <thread>

// --- SHARED MEMORY POINTER ---
// This will point to the specific RAM address of your Kotlin ByteBuffer.
// Volatile ensures C++ writes it immediately, not caching it in a CPU register.
volatile jlong* shared_can_data = nullptr;

// External C helper from your other files
extern "C" {
    #include "can_interface.h"
}

void read_can_loop() {
    // 1. Setup SocketCAN (Standard Linux CAN API)
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket Error");
        return;
    }

    strcpy(ifr.ifr_name, "can0");
    ioctl(s, SIOCGIFINDEX, &ifr);

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind Error");
        return;
    }

    struct can_frame frame;
    
    // 2. The High-Speed Loop
    while (true) {
        // Read a frame (Blocking call, waits for data)
        int nbytes = read(s, &frame, sizeof(struct can_frame));
        
        // If we have data AND the shared memory is initialized...
        if (nbytes > 0 && shared_can_data != nullptr) {
            
            // Mask out error flags to get the raw ID
            int id = frame.can_id & 0x7FF; 
            
            // Safety: Only write if ID fits in our allocated buffer (size 2048)
            if (id < 2048) {
                long long payload = 0;
                
                // Copy up to 8 bytes of data into the 64-bit integer
                // This handles 2-byte messages (speed) and 8-byte messages (IMU) automatically.
                std::memcpy(&payload, frame.data, std::min((int)frame.can_dlc, 8));
                
                // WRITE DIRECTLY TO KOTLIN'S MEMORY
                shared_can_data[id] = payload;
            }
        }
    }
}

extern "C" {

    // 1. INIT BUFFER: Kotlin calls this to say "Here is the RAM address to write to"
    JNIEXPORT void JNICALL
    Java_org_baja_dashboard_model_DataRepository_initBuffer(JNIEnv *env, jobject thiz, jobject buffer) {
        // Get the direct address of the ByteBuffer
        shared_can_data = (jlong*)env->GetDirectBufferAddress(buffer);
        std::cout << "DEBUG: Shared Buffer Initialized at " << shared_can_data << std::endl;
    }

    // 2. START: Starts the background listener
    JNIEXPORT void JNICALL
    Java_org_baja_dashboard_model_DataRepository_start(JNIEnv *env, jobject thiz) {
        // Run the C++ loop. Since Kotlin calls this inside Dispatchers.IO, 
        // it is safe to block here.
        std::cout << "DEBUG: Starting CAN Loop..." << std::endl;
        read_can_loop();
    }
    
    // --- LEGACY STUBS (To prevent crashes if old code calls them) ---
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getSpeed(JNIEnv *env, jobject thiz) { return 0.0; }
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getRPM(JNIEnv *env, jobject thiz) { return 0.0; }
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getTemperature(JNIEnv *env, jobject thiz) { return 0.0; }
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getFuel(JNIEnv *env, jobject thiz) { return 0.0; }
    
    // ... Add more stubs here if your app crashes looking for getFLAccel, etc.
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getFLAccel(JNIEnv *env, jobject thiz) { return 0.0; }
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getFLGyro(JNIEnv *env, jobject thiz) { return 0.0; }
    JNIEXPORT jdouble JNICALL Java_org_baja_dashboard_model_DataRepository_getFLSuspension(JNIEnv *env, jobject thiz) { return 0.0; }
}