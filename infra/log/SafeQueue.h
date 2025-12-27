
#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

// 线程安全的日志事件队列（生产者-消费者）
template <typename T>
class SafeQueue
{
public:
    SafeQueue(size_t max_size = 10000) : _maxSize(max_size), _isRunning(true) {}

    // 生产者入队（阻塞式：队列满时等待）
    bool Push(const T &data)
    {
        std::unique_lock<std::mutex> lock(this->_mutex);
        // 队列满且运行中，等待消费者消费
        this->_notFullCond.wait(lock, [this]()
                                { return !this->_isRunning || this->_queue.size() < this->_maxSize; });

        if (!this->_isRunning)
            return false; // 已停止，放弃入队
        this->_queue.push(data);
        this->_notEmptyCond.notify_one(); // 通知消费者有数据
        return true;
    }

    // 消费者出队（阻塞式：队列为空时等待）
    bool Pop(T &data)
    {
        std::unique_lock<std::mutex> lock(this->_mutex);
        this->_notEmptyCond.wait(lock, [this]()
                                 { return !this->_isRunning || !this->_queue.empty(); });

        if (!this->_isRunning && this->_queue.empty())
            return false; // 已停止且队空，退出
        data = this->_queue.front();
        this->_queue.pop();
        this->_notFullCond.notify_one(); // 通知生产者有空间
        return true;
    }

    // 优雅停止（不再接收新日志，处理完队列剩余内容）
    void Stop()
    {
        this->_isRunning = false;
        this->_notEmptyCond.notify_all(); // 唤醒所有等待的消费者
        this->_notFullCond.notify_all();  // 唤醒所有等待的生产者
    }

    // 获取当前队列大小（仅用于监控）
    std::size_t Size()
    {
        std::lock_guard<std::mutex> lock(this->_mutex);
        return this->_queue.size();
    }

private:
    std::queue<T> _queue;                  // 日志事件队列
    std::mutex _mutex;                     // 队列互斥锁
    std::condition_variable _notFullCond;  // 队列不满条件变量
    std::condition_variable _notEmptyCond; // 队列不空条件变量
    std::size_t _maxSize;                  // 队列最大容量（避免内存溢出）
    std::atomic<bool> _isRunning;          // 运行标志（原子变量，线程安全）
};

#endif // SAFE_QUEUE_H