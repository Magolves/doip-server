#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
  private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;

  public:
    ThreadSafeQueue() = default;

    ~ThreadSafeQueue() {
        stop();
    }

    // Disable copy operations
    ThreadSafeQueue(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    // Enable move operations
    ThreadSafeQueue(ThreadSafeQueue &&other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        queue_ = std::move(other.queue_);
        stopped_ = other.stopped_;
        other.stopped_ = true;
    }

    ThreadSafeQueue &operator=(ThreadSafeQueue &&other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mutex_, other.mutex_);
            queue_ = std::move(other.queue_);
            stopped_ = other.stopped_;
            other.stopped_ = true;
            cv_.notify_all();
        }
        return *this;
    }

    /**
     * @brief Push an item into the queue.
     *
     * @tparam U Type of the item to push
     * @param item Item to push into the queue
     */
    template <typename U>
    void push(U&& item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_)
                return;
            queue_.push(std::forward<U>(item));
        }
        cv_.notify_one();
    }

    bool pop(T &item, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!cv_.wait_for(lock, timeout, [this] {
                return !queue_.empty() || stopped_;
            })) {
            return false; // Timeout
        }

        if (stopped_ && queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Check if the queue is empty
     *
     * @return true if queue has no elements
     */
    bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Wait indefinitely for an item and pop it
     *
     * @param item Reference to store the popped item
     * @return true if item was popped, false if queue was stopped
     */
    bool waitAndPop(T &item) {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [this] {
            return !queue_.empty() || stopped_;
        });

        if (stopped_ && queue_.empty()) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /**
     * @brief Try to pop an item without blocking
     *
     * @param item Reference to store the popped item
     * @return true if item was popped, false if queue was empty or stopped
     */
    bool tryPop(T &item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty() || stopped_) {
            return false;
        }

        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /**
     * @brief Clear all items from the queue
     */
    void clear() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::queue<T> empty;
            std::swap(queue_, empty);
        }
        cv_.notify_all();
    }
};