#ifndef SIGINT_H
#define SIGINT_H

#include <functional>

/**
 * @brief Set a handler for SIGINT
 * @details On second SIGINT received, the program is forcefully terminated
 */
void set_sigint_handler(std::function<void()> handler);

#endif