#ifndef VULKAN_SAFEQUEUE_H
#define VULKAN_SAFEQUEUE_H

#include <queue>
#include <mutex>


template<class T>
class SafeQueue {
private:
    std::queue<T> queue;
    mutable std::mutex mutex;

public:
    SafeQueue() : queue(), mutex() {};

    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }

    void enqueue(const T& t) {
       std::unique_lock<std::mutex> lock(mutex);
       queue.push(t);
    }

    void enqueue(T&& t) {
       std::unique_lock<std::mutex> lock(mutex);
       queue.push(std::move(t));
    }

    T dequeue() {
       std::unique_lock<std::mutex> lock(mutex);
       if (!queue.empty()) {
          T message = std::move(queue.front());
          queue.pop();
          return message;
       }
       return T();
    }

    bool isEmpty() const { return queue.empty(); }

    size_t size() const { return queue.size(); }
};


#endif //VULKAN_SAFEQUEUE_H
