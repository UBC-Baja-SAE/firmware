#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <thread>
#include <cstring>
#include "can_processor.h"

extern "C"
{
    #include "can_interface.h"
    #include "can_bridge.h"
}

void poll()
{
    while (true)
    {
        CAN_Message msg;
        if (can_receive(socket_fd, &msg) == 1)
        {
            std::lock_guard<std::mutex> lock(queue_lock);
            message_queue.push(msg);
            cv.notify_one();
        }
    }
}

void process()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(queue_lock);
        cv.wait(lock, [] { return !message_queue.empty(); });

        CAN_Message msg = message_queue.front();
        message_queue.pop();

        lock.unlock();
        cv.notify_one();

        // set the data in the data map according to the message id
        int mod_id = msg.id % categories;

        // the CAN bus data is (for now) assumed to be little-endian
        memcpy(&data_map[mod_id], msg.data, msg.size);
    }
}

void start()
{
    can_init();

    can_filter_init(socket_fd, 0x000, 0x7f8);

    std::thread poll_thread(poll);
    std::vector<std::thread> workers;
    for (int i = 0; i < worker_count; i++) {
        workers.emplace_back(process);
    }
    poll_thread.join();
    for (auto& worker : workers) {
        worker.join();
    }
}