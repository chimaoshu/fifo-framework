#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <future>
#include <utility>
#include <functional>

#include "src/log/Log.hpp"

// 适用于多线程的原子性队列
template <typename T>
class AtomicQueue
{
private:
    std::queue<T> queue_;
    std::mutex mutex_;

public:
    AtomicQueue() = default;
    // 禁用赋值与拷贝构造
    AtomicQueue(const AtomicQueue &) = delete;
    AtomicQueue(AtomicQueue &&) = delete;
    AtomicQueue &operator=(const AtomicQueue &) = delete;
    AtomicQueue &operator=(AtomicQueue &&) = delete;

    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    int size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void push(T &t)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.emplace(t);
    }

    // 返回是否成功，内容赋值在in中
    bool pop(T &in)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 空队列
        if (queue_.empty())
            return false;

        // 非空
        in = std::move(queue_.front());
        queue_.pop();
        return true;
    }
};

// 线程池
class ThreadPool
{
private:
    // 工作线程类
    class ThreadWorker
    {
    private:
        int worker_id_;
        ThreadPool *thread_pool_;

    public:
        ThreadWorker(ThreadPool *pool, int id)
            : thread_pool_(pool), worker_id_(id) {}
        ~ThreadWorker() = default;

        void operator()()
        {
            std::function<void()> func; // 需要运行的函数
            bool pop_queue_success;     // 是否成功从队列中取出内容

            while (true)
            {
                // 取任务
                {
                    std::unique_lock<std::mutex> lock(thread_pool_->mutex_);

                    // 等待条件变量
                    while (!thread_pool_->shutdown_ && thread_pool_->task_queue_.empty())
                    {
                        Log::debug("worker" + std::to_string(worker_id_) + " start waiting for task");
                        thread_pool_->cond_.wait(lock);
                    }

                    if (thread_pool_->shutdown_)
                        break;

                    // 从线程池的任务队列中取出任务
                    pop_queue_success = thread_pool_->task_queue_.pop(func);
                }

                // 执行任务
                assert(pop_queue_success);
                if (pop_queue_success)
                    func();

                Log::debug("worker" + std::to_string(worker_id_) + " finish task");
            }
        }
    };

    bool shutdown_ = false;                         // 停止
    AtomicQueue<std::function<void()>> task_queue_; // 任务队列
    std::vector<std::thread> threads_;              // 线程列表

    std::mutex mutex_;
    std::condition_variable cond_;

public:
    ThreadPool(int num_threads)
    {
        if (num_threads <= 0)
        {
            std::string msg = "error threads num setting";
            UtilError::error_exit(msg, false);
        }

        // 初始化工作线程列表
        threads_.resize(num_threads);
        for (int i = 0; i < threads_.size(); i++)
            threads_[i] = std::thread(ThreadWorker(this, i));

        Log::debug("thread pool init success");
    }

    ~ThreadPool()
    {
        if (!shutdown_)
            shutdown();
    }

    // 禁用拷贝、赋值
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    // 停止
    void shutdown()
    {
        // 修改shutdown需要加锁，防止工作线程进入while后条件发生改变，永远shutdown不了
        {
            std::unique_lock<std::mutex> lock(mutex_);
            shutdown_ = true;
        }

        // 通知唤醒所有工作线程，让工作线程退出
        cond_.notify_all();

        // join所有线程
        for (int i = 0; i < threads_.size(); i++)
        {
            if (threads_[i].joinable())
                threads_[i].join();
            else
                Log::warn("thread with id" + std::to_string(i) + "is not joinable");
        }

        Log::info("thread pool is shutdown");
    }

    // 向线程池增加一个任务（函数），返回std::future<Func函数的返回类型>的future实例
    template <typename Func, typename... Args>
    auto submit(Func &&f, Args &&...args) -> std::future<decltype(f(args...))>
    {
        // 函数的返回类型
        using retType = decltype(f(args...));

        // 定义要执行的函数函数
        std::function<retType()> func = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);

        // 将函数包在packaged_task中，这样packaged_task运行会返回相应的future，把packaged_task包在共享指针中
        auto task_ptr = std::make_shared<std::packaged_task<retType()>>(func);

        // 用void类型函数包裹上述函数，方便统一送入队列
        std::function<void()> wrapper_func = [task_ptr]()
        { (*task_ptr)(); };

        // 送入任务队列
        task_queue_.push(wrapper_func);

        // 唤醒一个线程
        cond_.notify_one();

        // 返回future
        return task_ptr->get_future();
    }
};

#endif // __THREAD_POOL__