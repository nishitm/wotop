#include "logger.h"
#include <iostream>
#include <sstream>

logIt::logIt(LogLevel l) {
    if(l >= logLevel) {
        _buffer << "[" << logStrings[l] << "] ";
        toPrint = true;
    } else {
        toPrint = false;
    }
}

logIt::~logIt() {
    if (toPrint) {
        _buffer << std::endl;
        std::cout << _buffer.str();
    }
}
