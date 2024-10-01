//
// Created by Syl on 12/08/2024.
//
#include <mostly_harmless/utils/mostlyharmless_TaskThread.h>
#include <cassert>
#include <thread>
namespace mostly_harmless::utils {
    void TaskThread::perform() {
        if (m_isThreadRunning) return;
        if (!action) {
            assert(false);
            return;
        }

        auto weakThis = weak_from_this();
        auto actionWrapper = [weakThis]() -> void {
            if (auto self = weakThis.lock()) {
                self->m_isThreadRunning.store(true);
                self->action();
                self->m_isThreadRunning.store(false);
            }
        };
        m_threadShouldExit = false;
        std::thread worker{ std::move(actionWrapper) };
        worker.detach();
    }

    void TaskThread::sleep() {
        m_canWakeUp = false;
        std::unique_lock<std::mutex> lock(m_mutex);
        auto weakThis = weak_from_this();
        m_conditionVariable.wait(lock, [weakThis]() -> bool { if (auto self = weakThis.lock()) return self->m_canWakeUp; else return true; });
    }

    void TaskThread::wake() {
        std::lock_guard<std::mutex> guard{ m_mutex };
        m_canWakeUp = true;
        m_conditionVariable.notify_one();
    }

    void TaskThread::signalThreadShouldExit() {
        m_threadShouldExit.store(true);
    }

    bool TaskThread::threadShouldExit() const noexcept {
        return m_threadShouldExit;
    }

    bool TaskThread::isThreadRunning() const noexcept {
        return m_isThreadRunning;
    }
} // namespace mostly_harmless::utils