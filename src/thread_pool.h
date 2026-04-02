#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    // 创建线程池，指定线程数量
    ThreadPool(int thread_num);

    // 添加任务
    void add_task(std::function<void()> task);

    // 销毁线程池
    ~ThreadPool();

private:
    // 线程要执行的函数
    void worker();

    // 线程列表
    std::vector<std::thread> workers;

    // 任务队列
    std::queue<std::function<void()>> tasks;

    // 互斥锁
    std::mutex mtx;

    // 条件变量
    std::condition_variable cv;

    // 线程池是否关闭
    bool stop;
};

#endif