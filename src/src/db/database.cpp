#include "db/database.h"
#include "db/exceptions.h"
#include "parse/type.h"

#include <mutex>


void Database::add_table(const std::string& table_name, Table table){
    auto lock = std::unique_lock(tables_lock);
    
    if(tables.contains(table_name)){
        throw std::runtime_error("Table " + table_name + " already exists");
    }
    
    tables.emplace(table_name, std::move(table));
}

Table& Database::get_table(const std::string& table_name){
    try{
        return tables.at(table_name);
    }
    catch(std::out_of_range&){
        throw InvalidQuery("Table " + table_name + " does not exist");
    }
}

void Database::remove_table(const std::string& table_name){
    auto lock = std::unique_lock(tables_lock);
    
    if(!tables.contains(table_name)){
        throw std::runtime_error("Table " + table_name + " does not exist");
    }
    
    tables.erase(table_name);
}

std::string make_response(bool success, const std::string& message){
    return std::string(success ? "OK" : "ERROR") + " " + message;
}

std::string Database::process_query(const std::string& query){
    bool success = true;
    std::string message;
    try {
        message = _process_query(query);
    }
    catch (const std::exception& e) {
        success = false;
        message = e.what();
    }
    
    return make_response(success, message);
}

std::string Database::_process_query(const std::string& query){
    TokenStream stream(query);
    
    std::string command = stream.get_token(TokenType::Identifier);
    
    if(command == "CREATE"){
        stream.ignore_token("TABLE");
        return process_create_table(stream);
    }
    
    else if(command == "DROP"){
        stream.ignore_token("TABLE");
        return process_drop_table(stream);
    }
    else if(command == "SELECT"){
        return process_select(stream);
    }
    else if(command == "INSERT"){
        stream.ignore_token("INTO");
        return process_insert(stream);
    }
    else if(command == "DELETE"){
        return process_delete(stream);
    }
    else{
        throw InvalidQuery("Unknown query type");
    }
}

std::string Database::process_drop_table(TokenStream& stream){
    std::string table_name;
    
    if(stream.empty()){
        throw InvalidQuery("Invalid query format");
    }
    
    remove_table(table_name);
    
    return make_response(true, "Table " + table_name + " dropped");
}

std::string Database::process_create_table(TokenStream& stream){
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    std::vector<std::string> column_names;
    std::vector<Cell::DataType> column_types;
    
    stream.ignore_token("(");
    
    while(stream.peek_token().value != ")"){
        std::string name = stream.get_token(TokenType::Identifier);
        Cell::DataType type = string_to_type(stream.get_token(TokenType::Identifier));
        
        column_names.push_back(std::move(name));
        column_types.push_back(std::move(type));
    }
    
    stream.ignore_token(")");
    stream.ignore_token(";");
    
    Table table(std::move(column_names), std::move(column_types));
    
    add_table(table_name, std::move(table));
    
    return make_response(true, "Table " + table_name + " created");
}

/*
std::string Database::process_insert(TokenStream& stream){
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    stream.ignore_token("VALUES");
    
    std::vector<Cell> values;
    
    stream.ignore_token("(");
    
    while(stream.peek_token().value != ")"){
        values.push_back(parse_token(stream));
    }
    
    stream.ignore_token(")");
    stream.ignore_token(";");
    
    Table& table = get_table(table_name);
    
    table.add_row(std::move(values));
    
    return make_response(true, "Row inserted into table " + table_name);
}
*/

std::string Database::process_select(TokenStream&){ throw std::runtime_error("Not implemented");};
std::string Database::process_insert(TokenStream&){ throw std::runtime_error("Not implemented");};
std::string Database::process_delete(TokenStream&){ throw std::runtime_error("Not implemented");};