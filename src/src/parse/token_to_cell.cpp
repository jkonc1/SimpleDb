#include "parse/token_to_cell.h"
#include "parse/token_stream.h"

Cell parse_token_to_cell(const Token& token) {
    if(token.get_type() == TokenType::String){
        return Cell(token.get_value(), Cell::DataType::String);
    }
    
    // otherwise it must be number
    
    std::string content = token.get_value();
    if(content.find('.') != std::string::npos){
        return Cell(content, Cell::DataType::Float);
    }
    return Cell(content, Cell::DataType::Int);
}
