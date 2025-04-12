#ifndef SIGINT_H
#define SIGINT_H

#include <functional>

void set_sigint_handler(std::function<void()> handler);

#endif