#include "csv/csv.h"
#include "db/exceptions.h"

#include <sstream>

constexpr char SEPARATOR = ',';
constexpr char ESCAPE_SEQUENCE = '\\';
constexpr char NULL_ESCAPE = 'x';

void write_element(std::ostream& output, const VoidableString& element) {
    if(!element.has_value()){
        output << ESCAPE_SEQUENCE << NULL_ESCAPE; 
        return;
    }
    
    auto& content = element.value();
        
    for(char c : content){
        if(c==ESCAPE_SEQUENCE){
            output << ESCAPE_SEQUENCE << ESCAPE_SEQUENCE;
            continue;
        }
        
        if(c==SEPARATOR){
            output << ESCAPE_SEQUENCE << SEPARATOR;
            continue;
        }
        
        output << c;
    }
}

void write_csv(std::ostream& output, const VoidableTable& data) {
    for (const auto& row : data) {
        for (const auto& cell : row) {
            write_element(output, cell);
            output << ',';
        }
        output << '\n';
    }
}

VoidableString parse_word(std::istream& line){
    std::string result;
    
    while(true){
        auto current = line.get();
        
        if(current==EOF || current=='\n'){
            throw ParsingError("Unexpected end of line");
        }
        
        if(current==',') break;
        
        if(current == ESCAPE_SEQUENCE){
            auto next = line.get();
            
            switch(next){
            case ESCAPE_SEQUENCE:
                result += ESCAPE_SEQUENCE;
                break;
            case NULL_ESCAPE:
                if(!result.empty()){
                    throw ParsingError("Null field has additional content");
                }
                if(line.get() != SEPARATOR){
                    throw ParsingError("Null field has additional content");
                }
                return std::nullopt;
            case SEPARATOR:
                result+=SEPARATOR;
                break;
            default:
                throw ParsingError("Unknown escape sequence encountered"); 
            }
        }
        else{
           result+=current; 
        }
    }
    
    return result;
}

VoidableRow parse_csv_line(const std::string& line){
    VoidableRow result;
    
    std::stringstream stream(line);
    
    while(stream.peek() != EOF){
        result.push_back(parse_word(stream));
    }
    
    return result;
}

VoidableTable read_csv(std::istream& input) {
    VoidableTable data;
    std::string line;
    while (std::getline(input, line)) {
        data.push_back(parse_csv_line(line));
    }
    return data;
}