#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>

class InvalidQuery : public std::runtime_error {
public:
    InvalidQuery(const std::string& message) : std::runtime_error(message) {}
};

class InvalidConversion : public InvalidQuery {
public:
    InvalidConversion(const std::string& message) : InvalidQuery(message) {}
};

class ParsingError : public std::runtime_error {
public:
    ParsingError(const std::string& message) : std::runtime_error(message) { }
};

#endif