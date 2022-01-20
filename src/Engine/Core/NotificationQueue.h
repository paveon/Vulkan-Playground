#ifndef GAME_ENGINE_NOTIFICATION_QUEUE_H
#define GAME_ENGINE_NOTIFICATION_QUEUE_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <future>
#include <atomic>
#include <optional>
#include <vector>


class NotificationQueue {
    std::deque<std::pair<std::promise<void>, std::function<void()>>> m_Tasks;
    std::mutex m_Mutex;
    std::condition_variable m_Ready;
    bool m_Done = false;

public:
    void Done() {
        {
            std::unique_lock<std::mutex> lock{m_Mutex};
            m_Done = true;
        }
        m_Ready.notify_all();
    }

    auto TryPop(std::function<void()>& task, std::promise<void>& taskPromise) {
        std::unique_lock<std::mutex> lock{m_Mutex, std::try_to_lock};
        if (!lock || m_Tasks.empty())
            return false;

        taskPromise = std::move(m_Tasks.front().first);
        task = std::move(m_Tasks.front().second);
        m_Tasks.pop_front();
        return true;
    }

    template<typename F>
    auto TryPush(F&& f) -> std::optional<std::future<void>> {
        std::future<void> asyncResult;
        {
            std::unique_lock<std::mutex> lock{m_Mutex, std::try_to_lock};
            if (!lock) return {};
            m_Tasks.emplace_back(std::promise<void>(), std::forward<F>(f));
            asyncResult = m_Tasks.back().first.get_future();
        }
        m_Ready.notify_one();
        return asyncResult;
    }

    auto Pop(std::function<void()>& task, std::promise<void>& taskPromise) -> bool {
        std::unique_lock<std::mutex> lock{m_Mutex};
        while (m_Tasks.empty() && !m_Done)
            m_Ready.wait(lock);

        if (m_Tasks.empty())
            return false;

        task = std::move(m_Tasks.front().second);
        taskPromise = std::move(m_Tasks.front().first);
        m_Tasks.pop_front();
        return true;
    }

    template<typename F>
    auto Push(F&& f) -> std::future<void> {
        std::future<void> asyncResult;
        {
            std::unique_lock<std::mutex> lock{m_Mutex};
            m_Tasks.emplace_back(std::promise<void>(), std::forward<F>(f));
            asyncResult = m_Tasks.back().first.get_future();
        }
        m_Ready.notify_one();
        return asyncResult;
    }
};


class TaskSystem {
    const unsigned m_ThreadCount{std::thread::hardware_concurrency()};
    std::vector<std::thread> m_Threads;
    std::vector<NotificationQueue> m_Queues{m_ThreadCount};
    std::atomic<unsigned> m_Index{0};

    void Run(unsigned i) {
        while (true) {
            std::function<void()> f;
            std::promise<void> p;

            for (unsigned threadIdx = 0; threadIdx != m_ThreadCount; ++threadIdx) {
                if (m_Queues[(i + threadIdx) & m_ThreadCount].TryPop(f, p)) break;
            }
            if (!f && !m_Queues[i].Pop(f, p)) break;
            f();
            p.set_value();
        }
    }

public:
    TaskSystem() {
        for (size_t i = 0; i < m_ThreadCount; ++i) {
            m_Threads.emplace_back([&, i] {
                Run(i);
            });
        }
    }

    ~TaskSystem() {
        for (auto& queue : m_Queues) queue.Done();
        for (auto& thread : m_Threads) thread.join();
    }

    constexpr static unsigned K = 10;

    template<typename F>
    auto Async(F&& f) -> std::future<void> {
        unsigned taskIdx = m_Index++;

        for (unsigned i = 0; i != (m_ThreadCount * K); ++i) {
            unsigned queueIdx = (taskIdx + i) % m_ThreadCount;
            std::optional<std::future<void>> futureOpt = std::move(m_Queues[queueIdx].TryPush(std::forward<F>(f)));
            if (futureOpt.has_value()) {
                std::future<void> future = std::move(futureOpt.value());
                futureOpt.reset();
                return future;
            }
        }
        return std::move(m_Queues[taskIdx & m_ThreadCount].Push(std::forward<F>(f)));
    }
};


#endif //GAME_ENGINE_NOTIFICATION_QUEUE_H
