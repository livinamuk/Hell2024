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

void ThreadPool::pause() {
    std::lock_guard<std::mutex> lock(mtx);
    isPaused = true;
}
void ThreadPool::unpause() {
    std::lock_guard<std::mutex> lock(mtx);
    isPaused = false;
    cv.notify_all();
}
void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]() { return tasks.empty(); });
}