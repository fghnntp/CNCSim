#ifndef _EMC_MSG_Queue_H_
#define _EMC_MSG_Queue_H_
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

//A thread safe Queue template
template<typename T>
class MessageQueue {
public:
    // Add a message to the queue
    void push(const T& msg) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(msg);
        }
        cond_.notify_one();
    }

    // Remove and return the next message (blocks if empty)
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]{ return !queue_.empty(); });
        T msg = queue_.front();
        queue_.pop();
        return msg;
    }

    // Try to pop a message without blocking (returns std::nullopt if empty)
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T msg = queue_.front();
        queue_.pop();
        return msg;
    }

    // Check if the queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable cond_;
};
#endif
