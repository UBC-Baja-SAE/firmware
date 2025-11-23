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

        // the CAN bus data is (for now) assumed to be little-endian
        uint64_t message_data;
        memcpy(&message_data, msg.data, sizeof(message_data));

        observed_data[msg.id] = message_data;
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