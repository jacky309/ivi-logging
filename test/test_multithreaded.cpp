#include "test-common.h"
#include <pthread.h>
#include <thread>

typedef LogContextWithConsolePlusDLTIfAvailable LogContext;

LOG_DECLARE_DEFAULT_CONTEXT(myTestContext, "TEST", "Test context");

int main(int, char const**) {

    new std::thread([&]() {
        pthread_setname_np(pthread_self(), "MyThread2");
        log_warn() << "Test log from thread";
    });

    log_warn() << "Test from main";

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
