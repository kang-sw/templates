#pragma once
#include "safe_queue.hxx"
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <type_traits>
#include <thread>

namespace templates {
class timeout_exception : public std::exception {
public:
    explicit timeout_exception(char const* _Message)
        : exception(_Message)
    {
    }
};

template <typename Ty_>
class future_proxy {
    friend class thread_pool;

public:
    std::future<Ty_> get_future()
    {
        auto future = promise_->get_future();
        promise_.reset();
        return std::move(future);
    }

    future_proxy() = default;
    future_proxy(const future_proxy& other) = delete;
    future_proxy(future_proxy&& other) noexcept = default;
    future_proxy& operator=(const future_proxy& other) = delete;
    future_proxy& operator=(future_proxy&& other) noexcept = default;

private:
    std::shared_ptr<std::promise<Ty_>> promise_;
};

class thread_pool {
public:
    using clock = std::chrono::system_clock;

public:
    thread_pool(size_t task_queue_cap_ = 1024, size_t num_workers = std::thread::hardware_concurrency(), size_t concrete_worker_count_limit = 1024) noexcept;

    ~thread_pool();

public:
    void resize_worker_pool(size_t new_size);
    size_t num_workers() const { return num_workers_cached_; }
    size_t num_pending_task() const { return tasks_.size(); }
    size_t task_queue_capacity() const { return tasks_.capacity(); }
    size_t num_available_workers() const { return num_workers_cached_ - num_working_workers_; }
    clock::duration average_wait() const { return clock::duration(average_wait_.load()); }
    size_t num_max_workers() const { return num_max_workers_; }
    void num_max_workers(size_t value);

    template <typename Fn_, typename... Args_>
    decltype(auto) launch_task(Fn_&& f, Args_... args);

private:
    bool try_add_worker__();
    void pop_workers__(size_t count);
    void check_reserve_worker__(size_t threshold);

public:
    std::chrono::milliseconds launch_timeout_ms{1000};
    std::chrono::microseconds max_stall_interval_time{20000};
    std::chrono::microseconds max_task_wait_time{5000};

private:
    struct worker_t {
        struct atomic_bool_wrap_t {
            std::atomic_bool value;

            atomic_bool_wrap_t() = default;
            atomic_bool_wrap_t(const atomic_bool_wrap_t& other) = delete;
            atomic_bool_wrap_t(atomic_bool_wrap_t&& other) noexcept
            {
                value = other.value.load();
            }
            atomic_bool_wrap_t& operator=(const atomic_bool_wrap_t& other) = delete;
            atomic_bool_wrap_t& operator=(atomic_bool_wrap_t&& other) noexcept
            {
                value = other.value.load();
                return *this;
            }
        };
        std::thread thread;
        atomic_bool_wrap_t disposer;
    };

    struct task_t {
        std::function<void()> event;
    };

private:
    safe_queue<task_t> tasks_;
    std::vector<worker_t> workers_;
    mutable std::mutex worker_lock_;

    std::condition_variable event_wait_;
    mutable std::mutex event_lock_;

    std::atomic_size_t num_workers_cached_;
    std::atomic_size_t num_working_workers_;
    std::atomic_size_t num_max_workers_;

    std::atomic<clock::time_point> latest_active_ = clock::now();
    std::atomic<clock::time_point> latest_event_;
    std::atomic<int64_t> average_wait_;
};

template <typename Fn_, typename... Args_>
decltype(auto) thread_pool::launch_task(Fn_&& f, Args_... args)
{
    using callable_return_type = std::invoke_result_t<Fn_, Args_...>;
    using return_type = future_proxy<callable_return_type>;

    auto promise = std::make_shared<std::promise<callable_return_type>>();
    auto value_tuple = std::make_tuple(promise, f, std::tuple<Args_...>(args...));

    struct executor {
        decltype(value_tuple) arg;
        void operator()()
        {
            auto& [promise, f, arg_pack] = arg;

            // Storing exception features are only available on release build,
            //to improve debug
#if defined(NDEBUG)
            try {
#endif
                if constexpr (std::is_same_v<void, callable_return_type>) {
                    std::apply(f, std::move(arg_pack));
                    promise->set_value();
                }
                else {
                    promise->set_value(std::apply(f, std::move(arg_pack)));
                }
#if defined(NDEBUG)
            } catch (std::exception&) {
                do {
                    try {
                        // if future was already retrieved, below statement will throw.
                        // then do nothing, to mandate error handling to caller
                        auto v = promise->get_future();
                    } catch (std::exception&) {
                        break;
                    }

                    // else, given promise was expired before the future was retrieved.
                    // then this block simply rethrow the exception, which occurs program termination.
                    throw;
                } while (false);

                // if above statement did not thrown, it means the future was correctly
                //retrieved when it was launched.
                promise->set_exception(std::current_exception());
            }
#endif
        }
    };

    using std::chrono::system_clock;
    auto elapse_begin = system_clock::now();

    check_reserve_worker__(1);

    task_t task;
    task.event = executor{std::move(value_tuple)};
    latest_event_ = clock::now();

    while (!tasks_.try_push(std::move(task))) {
        if (system_clock::now() - elapse_begin > launch_timeout_ms) {
            throw timeout_exception{""};
        }

        std::this_thread::yield();
    }

    event_wait_.notify_one();

    return_type result;
    result.promise_ = promise;

    return result;
}

inline thread_pool::thread_pool(size_t task_queue_cap_, size_t num_workers, size_t worker_limit) noexcept
    : tasks_(task_queue_cap_)
    , num_max_workers_(worker_limit)
{
    resize_worker_pool(num_workers);
}

inline thread_pool::~thread_pool()
{
    pop_workers__(workers_.size());
}

inline void thread_pool::resize_worker_pool(size_t new_size)
{
    new_size = std::min(num_max_workers_.load(), new_size);
    if (new_size == 0) {
        throw std::invalid_argument{"Size 0 is not allowed"};
    }

    std::lock_guard<std::mutex> lock(worker_lock_);
    if (new_size > workers_.size()) {
        while (new_size != workers_.size()) {
            try_add_worker__();
        }
    }
    else if (new_size < workers_.size()) {
        pop_workers__(workers_.size() - new_size);
    }
}

inline void thread_pool::num_max_workers(size_t value)
{
    if (value == 0) {
        throw std::invalid_argument("0 is not allowed");
    }

    std::lock_guard<std::mutex> lock{worker_lock_};
    num_max_workers_ = value;

    if (value < workers_.size()) {
        pop_workers__(workers_.size() - value);
    }
}

inline bool thread_pool::try_add_worker__()
{
    if (workers_.size() >= num_max_workers_) {
        return false;
    }

    auto worker = [this, index = workers_.size()]() {
        task_t task;

        while (workers_[index].disposer.value == false) {
            if (tasks_.try_pop(task)) {
                auto now = clock::now();

                if (num_available_workers() == 1) {
                    auto wait_time = (now - latest_event_.load()).count();
                    auto average = average_wait_.load();

                    auto new_average = ((average * 7) + wait_time) / 8;
                    auto diff = new_average - average;
                    average_wait_.fetch_add(diff);
                }

                check_reserve_worker__(1);
                latest_active_ = now;
                latest_event_ = now;
                num_working_workers_.fetch_add(1);

                task.event();
                num_working_workers_.fetch_sub(1);
            }
            else {
                std::unique_lock<std::mutex> lock(event_lock_);
                event_wait_.wait(lock);
            }
        }
    };

    auto& wd = workers_.emplace_back();
    wd.disposer.value = false;
    wd.thread = std::thread(std::move(worker));

    num_workers_cached_ = workers_.size();
    return true;
}

inline void thread_pool::pop_workers__(size_t count)
{
    auto const begin = workers_.end() - count;
    auto const end = workers_.end();

    for (auto it = begin; it != end; ++it) {
        it->disposer.value.store(true);
    }
    event_wait_.notify_all();
    for (auto it = begin; it != end; ++it) {
        it->thread.join();
    }

    workers_.erase(begin, end);
    num_workers_cached_ = workers_.size();
}

inline void thread_pool::check_reserve_worker__(size_t threshold)
{
    if ( // reserve workers if required.
      num_available_workers() <= threshold
      && (clock::now() - latest_active_.load() > max_stall_interval_time
          || average_wait() > max_task_wait_time)) {
        resize_worker_pool((num_workers() & ~1) + 2);
    }
}
} // namespace templates
