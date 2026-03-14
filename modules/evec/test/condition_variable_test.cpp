// clang-format off
// RUN: %lotto %stress -r 10 -- %b
// clang-format on
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <thread>

std::condition_variable cv;
std::mutex cv_m; // This mutex is used for three purposes:
                 // 1) to synchronize accesses to i
                 // 2) to synchronize accesses to std::cerr
                 // 3) for the condition variable cv
int i = 0;

void
waits()
{
    std::unique_lock<std::mutex> lk(cv_m);
    std::cerr << "Waiting... \n";
    cv.wait(lk, [] {
        std::cerr << "checking\n";
        return i == 1;
    }); // yield
    std::cerr << "...finished waiting. i == 1\n";
}

void
signals()
{
    {
        std::lock_guard<std::mutex> lk(cv_m);
        std::cerr << "Notifying...\n";
    }
    cv.notify_all();

    for (int i = 0; i < 10; i++) {
        sched_yield();
    }

    {
        std::lock_guard<std::mutex> lk(cv_m);
        i = 1;
        std::cerr << "Notifying again...\n";
    }
    cv.notify_all();
}

int
main()
{
    std::thread t1(waits), t2(signals);
    t1.join();
    t2.join();
    return 0;
}
