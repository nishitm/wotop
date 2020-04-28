#ifndef LOGGER
#define LOGGER

#include <iostream>
#include <sstream>
#include <mutex>

enum LogLevel { DEBUG, VERB2, VERB1, INFO, WARN, ERROR};

const LogLevel logLevel = WARN;

class logIt {
public:
    logIt(LogLevel l);
    logIt(LogLevel l, const char *s);

    template <typename T>
    logIt & operator<<(T const & value __attribute__((unused))) {
        // _buffer << value;
        return *this;
    }

    ~logIt();

private:
    static std::ostringstream _buffer;
    bool toPrint;
    static std::mutex llock;
};

#define logger(...) logIt(__VA_ARGS__)

#endif
