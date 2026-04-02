#include "thread_pool.h"

ThreadPool::ThreadPool(int thread_num) : stop(false) {
    for (int i = 0; i < thread_num; ++i) {
        workers.emplace_back([this] {
            worker();
        });
    }
}

void ThreadPool::add_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return stop || !tasks.empty();
            });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stop = true;
    }
    cv.notify_all();
    for (auto &worker : workers) {
        worker.join();
    }
}