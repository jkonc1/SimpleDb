#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <string>

/**
 * @brief Check if a word is a reserved SQL keyword
 */
bool is_keyword(const std::string& word);

#endif