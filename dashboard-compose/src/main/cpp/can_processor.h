/**
 * @file can_processor.h
 */

 #ifndef CAN_PROCESSOR_H
 #define CAN_PROCESSOR_H 

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

extern "C"
{
    #include "can_interface.h"
    #include "can_bridge.h"
}

std::queue<CAN_Message> message_queue;

std::mutex queue_lock;

std::condition_variable cv;

const int worker_count = 2;

void poll();

void start();

int main();

#endif // CAN_PROCESS_H