#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <thread>

extern "C"
{
    #include "can_interface.h"
}

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
        CAN_Message msg;

        std::unique_lock<std::mutex> lock(queue_lock);
        cv.wait(lock, [] { return !message_queue.empty(); });

        msg = message_queue.front();
        message_queue.pop();

        lock.unlock();
        cv.notify_one();

        /* temporary sanity check, formatting the data as hex */
        std::cout << "processing message [" << std::hex << std::setw(3) << std::setfill('0') << msg.id << "]: ";
        for (int i = 0; i < msg.size; i++)
        {
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(msg.data[i]);
        }
        std::cout << std::endl;

        /* more processing */
    }
}

void start()
{
    can_init();

    can_filter_init(socket_fd, 0x123, 0x7ff);

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

int main()
{
    start();
}