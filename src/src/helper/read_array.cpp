#include "helper/read_array.h"

#include <parse/token_stream.h>

std::vector<Token> read_array(TokenStream& stream){
    std::vector<Token> result;
    
    if(stream.peek_token().value == ")"){
        return result;
    }
    
    while(true){
        auto value = stream.get_token();
        result.push_back(value);
        
        if(stream.peek_token().value == ")"){
            break;
        }
        
        stream.ignore_token(",");
    }
    
    return result;
}
