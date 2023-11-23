#pragma once
#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>

class ThreadPool {
public:
    ThreadPool(int numThreads) : numThreads(numThreads), stop(false) {
        for (int i = 0; i < numThreads; ++i) {
            threadPool.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [this]() { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    ~ThreadPool();

    template<typename Func, typename... Args>
    void addTask(Func&& func, Args&&... args) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.emplace([func, args...]() { func(args...); });
        }
        cv.notify_one();
    }

private:
    int numThreads;
    std::vector<std::thread> threadPool;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;
};
