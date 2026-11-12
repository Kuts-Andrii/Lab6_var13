#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

class Task {
public:
    struct promise_type {
        std::exception_ptr exception;

        Task get_return_object() noexcept {
            return Task{Handle::from_promise(*this)};
        }

        std::suspend_always initial_suspend() const noexcept {
            return {};
        }

        std::suspend_always final_suspend() const noexcept {
            return {};
        }

        void return_void() const noexcept {}

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    using Handle = std::coroutine_handle<promise_type>;

    Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, {});
        }
        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() {
        destroy();
    }

    void resume() {
        if (done()) {
            throw std::logic_error("Cannot resume a finished coroutine");
        }
        handle_.resume();
        if (auto error = handle_.promise().exception) {
            handle_.promise().exception = nullptr;
            std::rethrow_exception(error);
        }
    }

    bool done() const noexcept {
        return !handle_ || handle_.done();
    }

private:
    explicit Task(Handle handle) noexcept
        : handle_(handle) {}

    void destroy() noexcept {
        if (handle_) {
            handle_.destroy();
            handle_ = {};
        }
    }

    Handle handle_;
};