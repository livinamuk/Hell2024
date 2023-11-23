#include "ThreadPool.h"

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();
    for (auto& thread : threadPool) {
        thread.join();
    }
}