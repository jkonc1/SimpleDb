#ifndef READ_ARRAY_H
#define READ_ARRAY_H

#include "parse/token_stream.h"
#include <vector>

/**
 * @brief Read a parentheses-enclosed array of tokens from
 * @details The array elements are separated by commas. 
            The parentheses are not consumed.
 */
std::vector<Token> read_array(TokenStream& stream);

#endif