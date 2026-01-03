#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
  private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopped = false;

  public:
    ThreadSafeQueue() = default;

    ~ThreadSafeQueue() noexcept {
        stop();
    }

    // Disable copy operations
    ThreadSafeQueue(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;

    // Enable move operations
    ThreadSafeQueue(ThreadSafeQueue &&other) noexcept {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = std::move(other.m_queue);
        m_stopped = other.m_stopped;
        other.m_stopped = true;
    }

    ThreadSafeQueue &operator=(ThreadSafeQueue &&other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_queue = std::move(other.m_queue);
            m_stopped = other.m_stopped;
            other.m_stopped = true;
            m_cv.notify_all();
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
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopped)
                return;
            m_queue.push(std::forward<U>(item));
        }
        m_cv.notify_one();
    }

    bool pop(T &item, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (!m_cv.wait_for(lock, timeout, [this] {
                return !m_queue.empty() || m_stopped;
            })) {
            return false; // Timeout
        }

        if (m_stopped && m_queue.empty()) {
            return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();
    }

    size_t size() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    /**
     * @brief Check if the queue is empty
     *
     * @return true if queue has no elements
     */
    bool empty() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief Wait indefinitely for an item and pop it
     *
     * @param item Reference to store the popped item
     * @return true if item was popped, false if queue was stopped
     */
    bool waitAndPop(T &item) {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_cv.wait(lock, [this] {
            return !m_queue.empty() || m_stopped;
        });

        if (m_stopped && m_queue.empty()) {
            return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     * @brief Try to pop an item without blocking
     *
     * @param item Reference to store the popped item
     * @return true if item was popped, false if queue was empty or stopped
     */
    bool tryPop(T &item) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_queue.empty() || m_stopped) {
            return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    /**
     * @brief Clear all items from the queue
     */
    void clear() noexcept {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::queue<T> empty_queue;
            std::swap(m_queue, empty_queue);
        }
        m_cv.notify_all();
    }
};