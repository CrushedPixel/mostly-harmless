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
#include <iostream>

namespace mostly_harmless::utils {

    class Timer : public std::enable_shared_from_this<Timer> {
    private:
        struct Private {
            explicit Private() = default;
        };

        std::string name;
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
        }

    public:
        explicit Timer(Private, std::string name) : name(std::move(name)) {}

        static std::shared_ptr<Timer> create(std::string name) {
            return std::make_shared<Timer>(Private(), std::move(name));
        }

        ~Timer() noexcept {
            stop();
        }

        void run(int intervalMs) {
            if (!action) return;
            stop();
            stopRequested.store(false);
            timerThread = std::make_unique<std::thread>(&Timer::timerLoop, this, std::chrono::milliseconds(intervalMs));
        }

        void run(double frequency) {
            run(static_cast<int>(1000.0 / frequency));
        }

        void stop() {
            if (!timerThread) return;
            
            stopRequested.store(true);
            cv.notify_all();
            timerThread->join();
            timerThread.reset();
        }

        std::function<void(void)> action{ nullptr };
    };

} // namespace mostly_harmless::utils
#endif // MOSTLYHARMLESS_MOSTLYHARMLESS_TIMER_H
