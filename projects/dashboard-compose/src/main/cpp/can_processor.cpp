#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cstring>
#include "can_processor.h"
#include "can_bridge.h"

extern "C"
{
    #include "can_interface.h"
}

std::unordered_map<int, uint64_t> observed_data;

std::queue<CAN_Message> message_queue;

std::mutex queue_lock;

std::condition_variable cv;

const int worker_count = 2;

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

        uint64_t message_data = 0;

        int bytesToCopy = 8; // Default for Speed/RPM
        
        if (msg.id >= 0x100 && msg.id <= 0x181) {
            bytesToCopy = 2;
        }

        memcpy(&message_data, msg.data, bytesToCopy); 

        observed_data[msg.id] = message_data;
    }
}

void start()
{
    can_init();

    start_i2c_processor();

    can_filter_init(socket_fd, 0x000, 0x000);
    
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