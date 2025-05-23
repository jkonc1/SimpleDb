#include "db/database.h"
#include "db/exceptions.h"
#include "parse/keywords.h"
#include "parse/type.h"
#include "helper/read_array.h"
#include "db/variable_list.h"
#include "db/table_serialization.h"

#include <mutex>

void Database::add_table(const std::string& table_name, Table table){
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
    if(!tables.contains(table_name)){
        throw std::runtime_error("Table " + table_name + " does not exist");
    }

    tables.erase(table_name);
}

std::string make_response(const std::string& status, const std::string& message){
    return status + " " + message;
}

std::string Database::process_query(const std::string& query) noexcept{
    std::string status;
    std::string message;
    try {
        message = process_query_make_stream(query);
        status = "OK";
    }
    catch (const InvalidQuery& e){
        status = "ERR";
        message = e.what();
    }
    catch (const std::exception& e) {
        status = "EXC";
        message = e.what();
    }

    return make_response(status, message);
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
        auto lock = std::unique_lock(tables_lock);
        return process_create_table(stream);
    }

    if(command.like("DROP")){
        auto lock = std::unique_lock(tables_lock);
        return process_drop_table(stream);
    }

    if(command.like("SELECT")){
        auto lock = std::shared_lock(tables_lock);
        return process_select(stream);
    }

    if(command.like("INSERT")){
        auto lock = std::shared_lock(tables_lock);
        return process_insert(stream);
    }

    if(command.like("DELETE")){
        auto lock = std::shared_lock(tables_lock);
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
        if(is_keyword(name)){
            throw InvalidQuery("Invalid column name");
        }
        
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
    
    bool all_columns = false;

    if(stream.try_ignore_token("(")){
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
        all_columns = true;
    }

    stream.ignore_token("VALUES");

    stream.ignore_token("(");

    std::vector<Token> values = read_array(stream);

    stream.ignore_token(")");
    stream.ignore_token(";");
    
    if(all_columns){
        std::vector<std::string> value_strings;
        for(auto&& value : values){
            value_strings.push_back(std::move(value.value));
        }
        table.add_row(value_strings);
        
        return "Row inserted into table " + table_name;
    }

    std::map<std::string, std::string> column_value_map;

    if(column_names.size() != values.size()){
        throw InvalidQuery("Column name and value count mismatch");
    }
    
    for(size_t i = 0; i < column_names.size(); ++i){
        column_value_map[column_names[i]] = values[i].value;
    }

    table.add_row(column_value_map);

    return "Row inserted into table " + table_name;
}

static std::vector<std::string> read_projection_list(TokenStream& stream){
    std::vector<std::string> projection_expressions(1);

    while(true){
        Token token = stream.get_token();
        
        if(token.type == TokenType::Empty){
            throw InvalidQuery("No FROM statement found");
        }

        if(token.like("FROM")){
            break;
        }
        if(token.like(",")){
            projection_expressions.push_back("");
            continue;
        }

        if(!projection_expressions.back().empty()){
            projection_expressions.back() += " ";
        }
        projection_expressions.back() += token.get_raw();
    }
    
    return projection_expressions;
}

std::vector<std::pair<const Table&, std::string>> Database::read_selected_tables(TokenStream& stream){
    std::vector<std::pair<const Table&, std::string>> taken_tables;

    while(true){
        std::string table_name = stream.get_token(TokenType::Identifier);

        auto next_token = stream.peek_token();

        std::string alias;

        if(next_token.type == TokenType::Identifier && !is_keyword(next_token.value)){
            alias = stream.get_token().value;
        }
        else{
            alias = table_name;
        }

        const Table& table = get_table(table_name);

        taken_tables.emplace_back(table, alias);

        if(!stream.try_ignore_token(",")){
            break;
        }
    }
    
    return taken_tables;
}

std::vector<Table> Database::evaluate_select_group(TokenStream& stream, const VariableList& variables, Table& table){
    if(!stream.try_ignore_token("GROUP")){
        std::vector<Table> result;
        result.push_back(table.clone());
        return result;
    }
    
    stream.ignore_token("BY");
    std::vector<std::string> grouping_columns;

    while(true){
        std::string column_name = stream.get_token(TokenType::Identifier);
        grouping_columns.push_back(column_name);

        if(!stream.try_ignore_token(",")){
            break;
        }
    }

    auto groups = table.group_by(grouping_columns);

    if(!stream.try_ignore_token("HAVING")){
        return groups;
    }
    
    std::string having_condition;
    while(!stream.peek_token().like(";") && !stream.empty()){
        having_condition += stream.get_token().value + " ";
    }

    std::vector<Table> filtered_groups;
    for(auto&& group : groups){
        TokenStream stream(having_condition);

        if(group.evaluate_aggregate_condition(stream, variables, select_callback)){
            filtered_groups.push_back(std::move(group));
        }
    }
    
    return filtered_groups;
}

static bool has_aggregate(std::vector<std::string> expressions){
    auto contains = [](const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    };
    
    const std::vector<std::string> aggregate_functions = {"COUNT", "SUM", "AVG", "MAX", "MIN"};

    for(auto&& expr : expressions){
        auto upper = to_upper(expr);
        
        for(const auto& aggregate_function : aggregate_functions){
            if(contains(upper, aggregate_function)){
                return true;
            }
        }
    }
    
    return false;
}

Table Database::evaluate_select(TokenStream& stream, const VariableList& variables = {}){
    stream.ignore_token("SELECT");
    bool distinct = stream.try_ignore_token("DISTINCT");
    stream.try_ignore_token("ALL");

    auto projection_expressions = read_projection_list(stream);

    auto taken_tables = read_selected_tables(stream);

    Table combined_table = Table::cross_product(taken_tables);

    if(stream.try_ignore_token("WHERE")){
        combined_table.filter_by_condition(stream, variables, select_callback);
    }
    
    bool is_aggregate = has_aggregate(projection_expressions);
    
    if(stream.peek_token().like("GROUP")){
        is_aggregate = true;
    }

    std::vector<Table> groups = evaluate_select_group(stream, variables, combined_table);
    
    combined_table.clear_rows();
    Table result = combined_table.project(projection_expressions, variables);
    
    for(auto&& group : groups) {
        auto projected = group.project(projection_expressions, variables, is_aggregate);
        result.vertical_join(projected);
    }

    if(distinct){
        result.deduplicate();
    }

    return result;
}

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

    std::string table_name = stream.get_token(TokenType::Identifier);
    stream.ignore_token("WHERE");

    get_table(table_name).filter_by_condition(stream, {}, select_callback, true);
    stream.ignore_token(";");

    return "Rows deleted from table " + table_name;
}