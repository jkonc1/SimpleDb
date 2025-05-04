#ifndef READ_ARRAY_H
#define READ_ARRAY_H

#include "parse/token_stream.h"
#include <vector>

std::vector<Token> read_array(TokenStream& stream);

#endif