//
// Created by josh on 03/11/2020.
//
#ifdef __GNUC__
int main() {}
#else
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <experimental/coroutine>
#include <cppcoro/generator.hpp>
#include <cppcoro/async_generator.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/sync_wait.hpp>
#include <future>
#ifdef _WIN32
#include <windows.h>
namespace stdco=std::experimental;
auto operator co_await(std::chrono::system_clock::duration duration)
{
    class awaiter
    {
        static
        void CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE,
                                    void* Context,
                                    PTP_TIMER)
        {
            stdco::coroutine_handle<>::from_address(Context).resume();
        }
        PTP_TIMER timer = nullptr;
        std::chrono::system_clock::duration duration;
    public:

        explicit awaiter(std::chrono::system_clock::duration d)
                : duration(d)
        {}

        ~awaiter()
        {
            if (timer) CloseThreadpoolTimer(timer);
        }

        bool await_ready() const
        {
            return duration.count() <= 0;
        }

        bool await_suspend(stdco::coroutine_handle<> resume_cb)
        {
            int64_t relative_count = -duration.count();
            timer = CreateThreadpoolTimer(TimerCallback,
                                          resume_cb.address(),
                                          nullptr);
            bool success = timer != nullptr;
            SetThreadpoolTimer(timer, (PFILETIME)&relative_count, 0, 0);
            return success;
        }

        void await_resume() {}

    };
    return awaiter{ duration };
}
#endif
void print_time()
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);

    char mbstr[100];
    if (std::strftime(mbstr, sizeof(mbstr), "[%H:%M:%S] ", std::localtime(&time)))
    {
        std::cout << mbstr;
    }
}


[[noreturn]] cppcoro::generator<std::string> produce_items()
{
    while (true)
    {
        auto v = rand();
        using namespace std::string_literals;
        auto i = "item "s + std::to_string(v);
        print_time();
        std::cout << "produced " << i << '\n';
        co_yield i;
    }
}




namespace foo {

}
cppcoro::task<int> next_value()
{
    using namespace std::chrono_literals;
    co_await std::chrono::seconds(1 + rand() % 5);
    co_return rand();
}

[[noreturn]] cppcoro::async_generator<std::string> produce_items_async()
{
    while (true)
    {
        auto v = co_await next_value();
        using namespace std::string_literals;
        auto i = "item "s + std::to_string(v);
        print_time();
        std::cout << "produced " << i << '\n';
        co_yield i;
    }
}

cppcoro::task<> consume_items(int const n)
{
    int i = 1;
    auto seq = produce_items_async();
    for co_await (auto const& s : seq)
    {
        print_time();
        std::cout << "consumed " << s << '\n';
        if (++i > n) break;
    }
    co_return;

}
int main()
{
    auto gen = consume_items(5);
    cppcoro::sync_wait(gen);
}
#endif