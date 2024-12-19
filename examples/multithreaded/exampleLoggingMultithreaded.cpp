#include "logging-multithreaded.h"
#include <thread>

// If an application is not multi-instance, we can define its unique identifier
LOG_DEFINE_APP_IDS("MyAp", "This is a small application showing how to use ivi-logging");

// Instantiate a log context and define it as default for this module
LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "MAIN", "This is a description of that logging context");

static int const DURATION = 1000;

void loop(char const* threadName) {
    for (int i = DURATION; i > 0; i--) {
        log_info() << "Hello from " << threadName;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void thread1() {
    auto threadName = "Thread1";
    pthread_setname_np(pthread_self(), threadName);
    loop(threadName);
}

void thread2() {
    auto threadName = "Thread2";
    pthread_setname_np(pthread_self(), threadName);
    loop(threadName);
}

int main(int, char**) {
    log_info() << "Hello from main thread";
    std::thread t1(thread1);
    std::thread t2(thread2);
    t1.join();
    t2.join();
}
