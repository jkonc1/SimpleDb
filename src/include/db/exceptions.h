#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>

/**
 * @brief Exception thrown for invalid database queries
 */
class InvalidQuery : public std::runtime_error {
public:
    InvalidQuery(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown for invalid data conversions
 */
class InvalidConversion : public InvalidQuery {
public:
    InvalidConversion(const std::string& message) : InvalidQuery(message) {}
};

/**
 * @brief Exception thrown for errors in parsing CSV
 */
class ParsingError : public std::runtime_error {
public:
    ParsingError(const std::string& message) : std::runtime_error(message) { }
};

#endif