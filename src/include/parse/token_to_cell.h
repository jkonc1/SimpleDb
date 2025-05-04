#ifndef TOKEN_TO_CELL_H
#define TOKEN_TO_CELL_H

#include "db/cell.h"
#include "parse/token_stream.h"

Cell parse_token_to_cell(const Token& token);

#endif