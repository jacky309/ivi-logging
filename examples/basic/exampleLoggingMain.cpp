#include "exampleLogging.h"
#include <unistd.h>

#include "ivi-logging-types.h"
#include <iostream>

// If an application is not multi-instance, we can define its unique identifier
LOG_DEFINE_APP_IDS("MyAp", "This is a small application showing how to use ivi-logging");

// Instantiate a log context and define it as default for this module
LOG_DECLARE_DEFAULT_CONTEXT(mainContext, "MAIN", "This is a description of that logging context");

// Instantiate another context which can be used instead of the previous one if specified
LOG_DECLARE_CONTEXT(anotherContext, "CXT2", "Another context");

class MyClass2 {};

namespace MyNamespace {

// Instantiate a log context and define it as default for this module
LOG_DECLARE_DEFAULT_CONTEXT(defaultContextForSpecificNamespace, "MAI2", "This is a description of that logging context");

void MyFunction() {
    int8_t h[9]{1, 2, 3, 4, 5, 6, 7, 8, 9};
    log_debug() << h;
    log_debug().enableUnsupportedTypes() << std::make_tuple(MyClass2{}, 8, h);
    log_info() << "My function called";
}

} // namespace MyNamespace

struct MyClass {

    // Define the log context to be used for that class. This overrides any default context which might have previously be set
    LOG_DECLARE_CLASS_CONTEXT("CCTX", "This is a class-specific context");

    void doSomething() {
        log_debug() << "We are doing something"; // This log uses the class log context
    }
};

struct MySubClass : MyClass {

    // Define the log context to be used for that class. This overrides any default context which might have previously be set
    LOG_DECLARE_CLASS_CONTEXT("CC", "This is a class-specific context for our sub-class");

    void doSomethingElse() {
        log_debug() << "We are doing something else";
        log_verbose() << "This is a verbose log";
    }
};

struct MyClassWithImportedContext {

    // Define the log context to be used for that class. This overrides any default context which might have previously be set
    LOG_SET_CLASS_CONTEXT(anotherContext);

    void doSomething() {
        log_debug() << "We are doing something. Imported context"; // This log uses the class log context
    }
};

struct SecondClassWithImportedContext {

    // Define the log context to be used for that class. This overrides any default context which might have previously be set
    LOG_SET_CLASS_CONTEXT(anotherContext);

    void doSomething() {
        log_debug() << "We are doing something. Imported context"; // This log uses the class log context
    }
};

void myFunction() {

    log_debug() << static_cast<int8_t>(255);
    log_debug() << static_cast<uint8_t>(255);

    // Redefines the context to use for this scope (this function)
    LOG_DECLARE_DEFAULT_LOCAL_CONTEXT("CXT3", "Function-specific logging context");

    log_info() << "myFunction log 1";
    log_warn() << "myFunction log 2";

    std::array<uint8_t, 3> myArray{1, 2, 3};
    std::span sp{myArray};
    log_debug() << "array: " << sp;
    log_debug() << "span: " << myArray;

    std::map<uint8_t, std::string> myMap{{1, "aa"}, {3, "bb"}};
    log_debug() << "map: " << myMap;

    int i = 555;
    log_debug() << std::hex << i << std::dec << " / " << i;
    log_debug() << i << std::endl;

    log_with_context(getDefaultContext(), logging::LogLevel::Info) << std::hex << i << std::endl;

    auto dd = 0x123456789;
    log_info() << dd;
    log_info() << std::hex << dd;

    log_info() << std::variant<int, std::string>{"This is a variant"};
    log_info() << "Not defined optional: " << std::optional<std::string>{};
    log_info() << "Defined optional: " << std::optional<std::string>{"optional value"};

    log_info() << "Chrono duration in microseconds: " << std::chrono::microseconds{123};
    log_info() << "Chrono duration in milliseconds: " << std::chrono::milliseconds{123};
    log_info() << "Chrono duration in seconds: " << std::chrono::seconds{12};
}

int generateDataForLogging() {
    static int count = 0;
    printf("generateDataForLogging called %i time(s)\n", ++count);
    return 100;
}

int main(int, char const**) {

    log_debug().writeFormatted("This log is using a format string, similar to the printf syntax. This is an int : %i", 345);
    log_error().writeFormatted("Another way to use the printf variant %i", 7345).writeFormatted(". Done");

    log_error().writeFormatted("This one is mixed. Here is a float : ") << 5.7F;
    log_warn() << 5.7;
    log_warn() << "Test string";

    log_error() << std::string_view{"This is a string used via a string_view. XXXXX. This content should not be visible"}.substr(0, 41);

    myFunction();

    myFunction2();

    MySubClass o;
    o.doSomething();
    o.doSomethingElse();

    MyClassWithImportedContext o2;
    o2.doSomething();

    SecondClassWithImportedContext o3;
    o3.doSomething();

    std::string stdString = "That is a std::string";
    log_error().write("Values can be passed at once to the write method. ", stdString, " / ", 1234);

    std::vector<char> charVector;
    for (char i = '0'; i <= '0' + 40; i++)
        charVector.push_back(i);

    log_warn() << "This is a vector of chars : " << charVector;

    MyNamespace::MyFunction();

    std::vector<MyClass*> vectorOfPointers;
    vectorOfPointers.push_back(new MyClass());
    vectorOfPointers.push_back(new MyClass());

    log_debug() << vectorOfPointers;

    log_warn() << generateDataForLogging();
    log_verbose() << generateDataForLogging();

    log_info() << "A log with std::endl" << std::endl;
    log_info() << "A log with std::ends" << std::ends;
    log_info() << "A log with std::flush" << std::flush;

    static int const DURATION = 1000;

    log_info() << "Waiting " << DURATION << " seconds";

    for (int i = DURATION; i > 0; i--) {
        log_info() << i << " seconds before termination";
        sleep(1);
    }

    disableConsoleLogging();
    log_error() << "This log should not visible in the console";

    log_info() << "We are done. Bye";
}
