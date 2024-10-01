//
// Created by Syl on 12/08/2024.
//

#ifndef MOSTLYHARMLESS_MOSTLYHARMLESS_TIMER_H
#define MOSTLYHARMLESS_MOSTLYHARMLESS_TIMER_H
#include "mostlyharmless_Timer.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace mostly_harmless::utils {

    class Timer : public std::enable_shared_from_this<Timer> {
    private:
        struct Private {
            explicit Private() = default;
        };

        std::atomic<bool> running{ false };
        std::atomic<bool> stopRequested{ false };
        std::unique_ptr<std::thread> timerThread;
        std::mutex mutex;
        std::condition_variable cv;

        void timerLoop(std::chrono::milliseconds interval) {
            while (!stopRequested.load(std::memory_order_relaxed)) {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    if (cv.wait_for(lock, interval, [this] { return stopRequested.load(std::memory_order_relaxed); })) {
                        break;
                    }
                }
                if (action) {
                    action();
                }
            }
            running.store(false, std::memory_order_relaxed);
        }

    public:
        explicit Timer(Private) {}

        static std::shared_ptr<Timer> create() {
            return std::make_shared<Timer>(Private());
        }

        ~Timer() noexcept {
            stop();
        }

        void run(int intervalMs) {
            if (!action) return;
            stop();
            running.store(true, std::memory_order_relaxed);
            stopRequested.store(false, std::memory_order_relaxed);
            timerThread = std::make_unique<std::thread>(&Timer::timerLoop, this, std::chrono::milliseconds(intervalMs));
        }

        void run(double frequency) {
            run(static_cast<int>(1000.0 / frequency));
        }

        void stop() {
            stopRequested.store(true, std::memory_order_relaxed);
            cv.notify_all();
            if (timerThread) {
                timerThread->join();
                timerThread.reset();
            }
        }

        [[nodiscard]] bool isTimerRunning() const noexcept {
            return running.load(std::memory_order_relaxed);
        }

        std::function<void(void)> action{ nullptr };
    };

} // namespace mostly_harmless::utils
#endif // MOSTLYHARMLESS_MOSTLYHARMLESS_TIMER_H
