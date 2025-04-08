#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

namespace logger{
    template<class ... Args>
    void log(const Args&... args){
        ((std::cout << args << ' '), ...);
        std::cout << std::endl;
    }
}

#endif