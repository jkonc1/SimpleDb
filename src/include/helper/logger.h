#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

namespace logger{
    /**
     * @brief Log a message to standard output
     * @details The parameters are printed separated by spaces
     */
    template<class ... Args>
    void log(const Args&... args){
        ((std::cout << args << ' '), ...);
        std::cout << std::endl;
    }
}

#endif