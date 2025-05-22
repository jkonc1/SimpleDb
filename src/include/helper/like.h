#ifndef LIKE_H
#define LIKE_H

#include <string>

/**
 * @brief Check if a string matches a LIKE pattern
 * @details `_` matches any character, `%` any string
 */
bool is_like(const std::string& value, const std::string& pattern);

#endif