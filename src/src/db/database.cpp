#include "db/database.h"
#include "db/exceptions.h"
#include "parse/type.h"
#include "helper/read_array.h"
#include "db/variable_list.h"
#include "db/table_serialization.h"

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

std::string Database::process_query(const std::string& query) noexcept{
    bool success = true;
    std::string message;
    try {
        message = process_query_make_stream(query);
    }
    catch (const std::exception& e) {
        success = false;
        message = e.what();
    }
    
    return make_response(success, message);
}

std::string Database::process_query_make_stream(const std::string& query){
    TokenStream stream(query);
    
    auto result = process_query_pick_type(stream);
    
    stream.assert_end();
    
    return result;
}

std::string Database::process_query_pick_type(TokenStream& stream){
    
    Token command = stream.peek_token();
    
    if(command.like("CREATE")){
        return process_create_table(stream);
    }
    
    if(command.like("DROP")){
        return process_drop_table(stream);
    }
    
    if(command.like("SELECT")){
        return process_select(stream);
    }
    
    if(command.like("INSERT")){
        return process_insert(stream);
    }
    
    if(command.like("DELETE")){
        return process_delete(stream);
    }
    
    throw InvalidQuery("Unknown query type");
}

std::string Database::process_drop_table(TokenStream& stream){
    stream.ignore_token("DROP");
    stream.ignore_token("TABLE");
    
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    stream.ignore_token(";");
    
    remove_table(table_name);
    
    return "Table " + table_name + " dropped";
}

std::string Database::process_create_table(TokenStream& stream){
    stream.ignore_token("CREATE");
    stream.ignore_token("TABLE");
    
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    std::vector<std::pair<Cell::DataType, std::string>> columns;
    
    stream.ignore_token("(");
    
    while(true){
        std::string name = stream.get_token(TokenType::Identifier);
        Cell::DataType type = string_to_type(stream.get_token(TokenType::Identifier));
        
        columns.emplace_back(type, std::move(name));
        
        std::string separator = stream.get_token(TokenType::SpecialChar);
        if(separator == ")"){
            break;
        }
        
        if(separator != ","){
            throw InvalidQuery("Invalid column separator");
        }
    }
    
    stream.ignore_token(";");
    
    Table table(columns);
    
    add_table(table_name, std::move(table));
    
    return "Table " + table_name + " created";
}

std::string Database::process_insert(TokenStream& stream){
    stream.ignore_token("INSERT");
    stream.ignore_token("INTO");
    
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    Table& table = get_table(table_name);
    
    std::vector<std::string> column_names;
    
    if(stream.peek_token().value == "("){
        stream.ignore_token("(");
        auto column_name_tokens = read_array(stream);
        
        for(const auto& token : column_name_tokens){
            if(token.type != TokenType::Identifier){
                throw InvalidQuery("Invalid column name");
            }
            column_names.push_back(token.value);
        }
        
        stream.ignore_token(")");
    }
    else{
        for(const auto& column : table.get_columns()){
            column_names.push_back(column.name);
        }
    }
    
    stream.ignore_token("VALUES");
    
    stream.ignore_token("(");
    
    std::vector<Token> values = read_array(stream);
    
    stream.ignore_token(")");
    stream.ignore_token(";");
    
    std::map<std::string, std::string> column_value_map;
    
    for(size_t i = 0; i < column_names.size(); ++i){
        column_value_map[column_names[i]] = values[i].value;
    }
    
    table.add_row(column_value_map);
    
    return "Row inserted into table " + table_name;
}

Table Database::evaluate_select(TokenStream& stream, const VariableList& variables = {}){
    stream.ignore_token("SELECT");
    
    [[maybe_unused]] bool distinct = stream.try_ignore_token("DISTINCT");
    
    stream.try_ignore_token("ALL");
    
    std::vector<std::string> selected_expressions(1);
    
    while(true){
        Token token = stream.get_token();
        
        if(token.like("FROM")){
            break;
        }
        if(token.like(",")){
            selected_expressions.push_back("");
            continue;
        }
        
        selected_expressions.back() += token.value;
    }
    
    std::vector<std::pair<const Table&, std::string>> taken_tables;
    
    while(true){
        std::string table_name = stream.get_token(TokenType::Identifier);
        
        auto next_token = stream.peek_token();
        
        std::string alias;
        
        if(next_token.type == TokenType::Identifier && !next_token.like("WHERE")){
            alias = stream.get_token().value;
        }
        else{
            alias = table_name;
        }
        
        const Table& table = get_table(table_name);
        
        taken_tables.emplace_back(table, alias);
        
        if(stream.peek_token().type != TokenType::Identifier || stream.peek_token().like("WHERE")){
            break;
        }
    }
    
    Table combined_table = Table::cross_product(taken_tables);
    
    if(stream.try_ignore_token("WHERE")){
        combined_table.filter_by_condition(stream, variables, select_callback);
    }
    
    return combined_table;
    std::vector<Table> groups;
    
    if(stream.try_ignore_token("GROUP")){
        stream.ignore_token("BY");
        
        std::vector<std::string> grouping_columns;
        
        while(true){
            std::string column_name = stream.get_token(TokenType::Identifier);
            
            grouping_columns.push_back(column_name);
            
            if(!stream.try_ignore_token(",")){
                break;
            }
        }
        
        groups=combined_table.group_by(grouping_columns);
    }
    else{
        groups.push_back(std::move(combined_table));
    }
    
    if(stream.try_ignore_token("HAVING")){
        std::vector<Table> filtered_groups;
        for(auto&& group : groups){
            if(group.evaluate_aggregate_condition(stream, variables, select_callback)){
                filtered_groups.push_back(std::move(group));
            }
            
            groups = std::move(filtered_groups);
        }
    }
    
    std::vector<Table> projected_tables;
    
    if(selected_expressions == std::vector<std::string>{"*"}){
        projected_tables = std::move(groups);
    }
    else {
        for(auto&& group : groups){
            
            Table projection = group.project(selected_expressions, variables);
            
            projected_tables.push_back(std::move(projection));
        }
    }
    
    Table result = combined_table.project(selected_expressions, variables);
    
    for(const auto& group : projected_tables){
        result.vertical_join(group);
    }
    
    if(distinct){
        result.deduplicate();
    }
    
    return result;
};

std::string Database::process_select(TokenStream& stream){
    Table table = evaluate_select(stream);
    stream.ignore_token(";");
    
    std::ostringstream response;
    
    serialize_table(table, response);
    
    return response.str();
}

std::string Database::process_delete(TokenStream& stream){
    stream.ignore_token("DELETE");
    
    stream.ignore_token("FROM");
    
    // TODO negate
     
    std::string table_name = stream.get_token(TokenType::Identifier);
    
    stream.ignore_token("WHERE");
    
    get_table(table_name).filter_by_condition(stream, {}, select_callback);
    
    return "Rows deleted from table " + table_name;
};